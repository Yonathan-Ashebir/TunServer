//
// Created by yoni_ash on 6/25/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_CLIENTHANDLER_H
#define TUNSERVER_CLIENTHANDLER_H

#include <utility>

#include "../Include.h"
#include "../tunnel/DisposableTunnel.h"
#include "../sessions2/TCPSession.h"

class ClientHandler {
public:
    using OnTunnelRequest = function<void(sockaddr_storage &addr, unsigned int clientId)>;

    constexpr static chrono::seconds NO_TUNNEL_TIMEOUT{120};
    constexpr static unsigned int MAXIMUM_TUNNELS{100};
    constexpr static unsigned int MAXIMUM_TCP_SESSIONS{500};
    constexpr static unsigned int HEALTHY_TUNNELS_COUNT{5};

    explicit ClientHandler(OnTunnelRequest callback = [](sockaddr_storage &, unsigned int) {}) : data(
            new Data{std::move(callback)}) {
    }

    ClientHandler(ClientHandler &) = default;

    ClientHandler(ClientHandler &&) = default;

    ClientHandler &operator=(const ClientHandler &) = default;

    ClientHandler &operator=(ClientHandler &&) = default;

    bool isActive() {
        return data->started;
    }

    void stop() {
        unique_lock lock(data->startedMutex);
        if (data->started) {
            data->shouldStop = true;
            data->waitingObject.notify_one();
        }
    }

    bool addTunnelSocket(TCPSocket &sock) {
        if (!data->started)return false;
        unique_lock lock(data->candidatesMutex, defer_lock);
        unique_lock startedLock(data->startedMutex, defer_lock);
        std::lock(lock, startedLock);
        if (data->tunnelCandidates.size() + data->openTunnelsCount == MAXIMUM_TUNNELS)return false;
        data->tunnelCandidates.emplace_back(sock);
        data->waitingObject.notify_one();
        return true;
    }

    void start(unsigned int id) {
        if (data->started)throw BadException("Handler is already running");
        data->started = true;
        data->shouldStop = false;
        data->clientId = id;
        thread th{[&] {
            handleClient();
        }};
        th.detach();
    }

    void setOnTunnelRequest(OnTunnelRequest callback) {
        data->tunnelRequester = std::move(callback);
    }

    OnTunnelRequest &getOnTunnelRequest() {
        return data->tunnelRequester;
    }


private:
    struct TunnelCandidate {
        TCPSocket socket;

        explicit TunnelCandidate(TCPSocket &sock) : socket(sock) {
        }
    };

    struct Data {
        OnTunnelRequest tunnelRequester;
        unsigned int clientId{};
        bool shouldStop{true};
        bool started{};
        mutex startedMutex{};
        condition_variable waitingObject{};

        socket_t maxFD{};
        fd_set sendSet{};
        fd_set receiveSet{};
        fd_set errorSet{};

        mutex candidatesMutex{};
        list<TunnelCandidate> tunnelCandidates{};
        list<DisposableTunnel> tunnels{};
        unsigned int openTunnelsCount{};
        list<TCPSession> tcpSessions{};
        unsigned int openTCPSessionsCount{};
        unsigned int tunnelIDCount{};

        explicit Data(OnTunnelRequest callback) : tunnelRequester(std::move(callback)) {
            FD_ZERO(&sendSet);
            FD_ZERO(&receiveSet);
            FD_ZERO(&errorSet);
        }
    };

    shared_ptr<Data> data;

    void handleClient() {
        constexpr static timeval tv{0, 10000};
        constexpr static chrono::milliseconds TICK_DELAY{20};

        const auto pred = [&]() -> bool {
            return data->openTunnelsCount > 0 || !data->tunnelCandidates.empty() || data->shouldStop;
        };

//        const auto sendNewTunnelRequest = [&] {
//            auto tunnel = data->tunnels.begin();
//            auto ind = 0;
//            while (ind < data->openTunnelsCount && !tunnel->sendNewTunnelRequest()) {
//                ind++;
//                tunnel++;
//            }
//            if (ind == data->openTunnelsCount)
//#ifdef LOGGING
//                cout << "Could not send 'new tunnel' request" << endl;
//#endif
//        };

        const auto sendDeleteTunnelRequest = [&](unsigned int id) {
            auto tunnel = data->tunnels.begin();
            auto ind = 0;
            while (ind < data->openTunnelsCount && !tunnel->sendDeleteTunnelRequest(id)) {
                ind++;
                tunnel++;
            }
            if (ind == data->openTunnelsCount)
#ifdef LOGGING
                cout << "Could not send delete tunnel request for the tunnel with id: " << id << endl;
#endif
            if (data->openTunnelsCount < HEALTHY_TUNNELS_COUNT) {
                /*request for a new tunnel*/
                auto count = data->openTunnelsCount;
                ind = 0;
                tunnel = data->tunnels.begin();
                while (ind < data->openTunnelsCount && count) {
                    while (count && tunnel->sendNewTunnelRequest()) {
                        count--;
                    }
                    ind++;
                    tunnel++;
                }
            }

        };
        CompressedTCPPacket tcpPacket{1000};
        auto sendTCPPacketBack = [&](CompressedTCPPacket &packet) {
            auto tunnel = data->tunnels.begin();
            int count{};
            while (count < data->openTunnelsCount && !tunnel->writePacket(packet)) {
                tunnel++;
                count++;
            }
#ifdef LOGGING
            if (count == data->openTunnelsCount)cout << "Could not send back tcp packet" << endl;
#endif
        };

        chrono::steady_clock::time_point lastTickTime{};
        auto startTunnelsCount = data->openTunnelsCount, startTCPSessionsCount = data->openTCPSessionsCount;

        while (true) {
            unique_lock lock(data->startedMutex);
            if (data->shouldStop) {
                data->started = false;
                break;
            }

            if (!data->waitingObject.wait_for(lock, NO_TUNNEL_TIMEOUT,
                                             pred)) {
                data->started = false;
                break;
            }

            if (data->shouldStop) {
                data->started = false;
                break;
            }
            lock.unlock();

            if (!data->tunnelCandidates.empty()) {
                unique_lock candidatesLock(data->candidatesMutex);
                for (auto &candidate: data->tunnelCandidates) {
                    if (data->openTunnelsCount == MAXIMUM_TUNNELS)break;
                    auto id = data->tunnelIDCount;
                    try {
                        candidate.socket.sendObject(toNetworkByteOrder(id));
                        data->tunnelIDCount++;
                    } catch (exception &e) {
#ifdef LOGGING
                        cout << "Sending the tunnel's id to the client failed / tunnel candidate failed\n  what(): "
                             << e.what() << endl;
#endif
                        continue;
                    }

                    if (data->openTunnelsCount == data->tunnels.size()) {
                        DisposableTunnel tunnel{id, candidate.socket, data->receiveSet, data->sendSet,
                                                data->errorSet};
                        initTunnel(tunnel);
                        data->tunnels.push_back(tunnel);
                    } else next(data->tunnels.begin(), data->openTunnelsCount++)->reset(id, candidate.socket);
                    data->openTunnelsCount++;
                }
                data->tunnelCandidates.clear();
            }

            if (data->openTunnelsCount != startTunnelsCount || data->openTCPSessionsCount != startTCPSessionsCount) {
                evalMaxFD();
            }

            startTunnelsCount = data->openTunnelsCount, startTCPSessionsCount = data->openTCPSessionsCount;

            fd_set snd = data->sendSet, rcv = data->receiveSet, err = data->errorSet;
            timeval tvCpy = tv;
            auto count = select(data->maxFD, &rcv, &snd, &err, &tvCpy);

            auto tunnel = data->tunnels.begin();
            for (auto remainingTunnels = data->openTunnelsCount;
                 remainingTunnels > 0 && count > 0; remainingTunnels--) {
                auto &sock = tunnel->getSocket();
                if (sock.isSetIn(rcv)) {
                    count--;
                    auto packets = tunnel->readPackets();
                    for (auto &p: packets) {
                        switch (p.getProtocol()) {
                            case IPPROTO_TCP: {
                                bool handled{};
                                for (auto &tcpSession: data->tcpSessions) {
                                    if ((handled = tcpSession.receiveFromClient(
                                            *reinterpret_cast<CompressedTCPPacket *>(&p), sendTCPPacketBack)))
                                        break;
                                }
                                if (!handled) {
                                    if (data->tcpSessions.size() == MAXIMUM_TCP_SESSIONS) {
#ifdef LOGGING
                                        cout << "TCP packet was not handled because max tcp sessions count was reached"
                                             << endl;
#endif
                                        break;
                                    }
                                    /*a new tcp session object is inserted*/
                                    auto tcpSession = data->tcpSessions.emplace_back(data->receiveSet, data->sendSet,
                                                                                     data->errorSet);
                                    initTCPSession(tcpSession);
                                    tcpSession.receiveFromClient(
                                            *reinterpret_cast<CompressedTCPPacket *>(&p), sendTCPPacketBack);
                                    if (tcpSession.getState() != TCPSession::CLOSED)data->openTCPSessionsCount++;
                                }
                                break;
                            }

                            default: {
#ifdef LOGGING
                                cerr << "Unimplemented ip protocol: " << p.getProtocol() << endl;
#endif
                            }
                        }
                    }
                }

                if (sock.isSetIn(snd)) {
                    count--;
                    tunnel->flushPackets();
                }

                if (sock.isSetIn(err)) {
                    /*deactivates a tunnel*/
                    count--;
                    tunnel->close();
                    data->openTunnelsCount--;
                    auto next = std::next(tunnel, 1);
                    data->tunnels.splice(data->tunnels.end(), data->tunnels, tunnel);
                    sendDeleteTunnelRequest(tunnel->getId());
                    tunnel = next;
                } else {
                    tunnel++;
                }
            }

            auto tcpSession = data->tcpSessions.begin();
            for (auto remainingTCPSessions = data->openTCPSessionsCount;
                 remainingTCPSessions > 0 && count > 0; remainingTCPSessions--) {
                auto &sock = tcpSession->getSessionSocket();
                if (sock.isSetIn(rcv)) {
                    count--;
                    tcpSession->receiveFromServer(tcpPacket, sendTCPPacketBack);
                }
                if (sock.isSetIn(snd)) {
                    count--;
                    tcpSession->flushDataToServer(tcpPacket, sendTCPPacketBack);
                }
                if (sock.isSetIn(err)) {
                    /*deactivates a tcp session sending a reset segment*/
                    count--;
                    tcpSession->onSocketError(tcpPacket, sendTCPPacketBack);
                    auto next = std::next(tcpSession, 1);
                    data->tcpSessions.splice(data->tcpSessions.end(), data->tcpSessions, tcpSession);
                    tcpSession = next;
                    data->openTCPSessionsCount--;
                }
            }

#ifdef STRICT_MODE
            assert(count == 0);
#endif

            auto now = chrono::steady_clock::now();
            if (now - lastTickTime > TICK_DELAY) {
                lastTickTime = now;
                tunnel = data->tunnels.begin();
                for (auto remainingTunnels = data->openTunnelsCount;
                     remainingTunnels > 0; remainingTunnels--) {
                    if (tunnel->checkStatus()) {
                        tunnel->flushPackets();
                        tunnel++;
                    } else {
                        /*deactivates a tunnel*/
                        auto next = std::next(tunnel, 1);
                        data->tunnels.splice(data->tunnels.end(), data->tunnels, tunnel);
                        data->openTunnelsCount--;
                        sendDeleteTunnelRequest(tunnel->getId());
                        tunnel = next;
                    }
                }

                tcpSession = data->tcpSessions.begin();
                for (auto remainingTCPSessions = data->openTCPSessionsCount;
                     remainingTCPSessions > 0; remainingTCPSessions--) {
                    if (tcpSession->checkStatus()) {
                        tcpSession->flushDataToClient(tcpPacket, sendTCPPacketBack);
                        tcpSession++;
                    } else {
                        /*deactivates a tcp session silently*/
                        auto next = std::next(tcpSession, 1);
                        data->tcpSessions.splice(data->tcpSessions.end(), data->tcpSessions, tcpSession);
                        tcpSession = next;
                        data->openTCPSessionsCount--;
                    }
                }
            }
        }
    }

    inline void evalMaxFD() {
#ifndef _WIN32
        socket_t result{};
        for (auto t: data->tunnels) {
            if (t.isOpen()) result = max(t.getSocket().getFD() + 1, result);
        }
        for (auto ts: data->tcpSessions) {
            if (ts.getState() != TCPSession::CLOSED)result = max(ts.getSessionSocket().getFD() + 1, result);
        }
        data->maxFD = result;
#endif
    };

    inline void initTCPSession(TCPSession &session) {
//todo ...
    }

    inline void initTunnel(DisposableTunnel &tunnel) {
        tunnel.setOnNewTunnelResponse([&](sockaddr_storage &addr) {
            data->tunnelRequester(addr, data->clientId);
        });

        tunnel.setOnDeleteTunnelRequest([&](unsigned int id) {
            for (auto ind = 0; ind < data->openTunnelsCount; ind++) {
                auto tunnel = next(data->tunnels.begin(), ind);
                if (tunnel->getId() == id) {
                    /*closing tunnel here*/
                    tunnel->close();
                    //IMP: moving to the back here is not safe, as the 'readPackets' loop might be on this very iterator
                    break;
                }
            }
        });

        tunnel.setOnUnsentData([&](DisposableTunnel::UnsentData &unsentData) {
            for (int ind = 0; ind < data->openTunnelsCount && unsentData.remaining; ind++) {
                next(data->tunnels.begin(), ind)->sendUnsentData(unsentData);
            }
        });
    }
};


#endif //TUNSERVER_CLIENTHANDLER_H

#pragma clang diagnostic pop