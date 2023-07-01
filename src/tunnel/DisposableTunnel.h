//
// Created by yoni_ash on 6/25/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_DISPOSABLETUNNEL_H
#define TUNSERVER_DISPOSABLETUNNEL_H

#include <utility>

#include "../Include.h"
#include "CompressedTunnel.h"

class DisposableTunnel : public CompressedTunnel {
public:
    struct UnsentData {
        char *buffer;
        unsigned int size;
        unsigned int offset;
        unsigned int writeNext{offset};
        bool completed{};

        UnsentData(char *buf, unsigned int size) : UnsentData(buf, size, 0) {};

        UnsentData(char *buf, unsigned int size, unsigned int off) : buffer(buf), size(size), offset(off) {}
    };

    using OnNewTunnelResponse = function<void(sockaddr_storage &)>;
    using OnDeleteTunnelRequest = function<void(unsigned int)>;
    using OnUnsentData = function<void(UnsentData &data)>;
    using OnNewTunnelRequest = function<void(void)>;

    inline explicit DisposableTunnel(TCPSocket &sock, fd_set &rcv, fd_set &snd, fd_set &err,
                                     unsigned int size = MIN_SS * 5) : CompressedTunnel(sock),
                                                                       data(new Data(size, rcv, snd, err)) {
        socket.setIn(rcv);
        socket.setIn(err);
        //todo: set sock timeout
        //todo: handle socket errors
    };

    bool writePacket(CompressedIPPacket &packet) override {
        if (1 + sizeof(DataSegment) + packet.getLength() > sendAvailable()) {
            flushPackets();
            if (1 + sizeof(DataSegment) + packet.getLength() > sendAvailable()) return false;
        }
        data->sendBuffer[data->sendNext++] = DATA_SEGMENT;

        DataSegment segment{toNetworkByteOrder(packet.getLength())};
        memcpy(data->sendBuffer + data->sendNext, &segment, sizeof(DataSegment));
        data->sendNext += sizeof(DataSegment);

        data->sendNext += packet.writeTo(data->sendBuffer + data->sendNext, sendAvailable());
        socket.setIn(data->sendSet);
        return true;
    };

    void flushPackets() {
        if (!isOpen() || data->written == data->sendNext)return;
        auto sock = getTCPSocket();
        try {
            auto res = sock.sendIgnoreWouldBlock(data->sendBuffer + data->written,
                                                 (int) (data->sendNext - data->written));
            if (res > 0) {
                data->written += res;
                data->lastSuccessfulSendTime = chrono::steady_clock::now();
                advanceLastPacketSent();
                if (data->written == data->sendNext) {
                    data->incompleteSend = false;
                    data->written = 0;
                    data->sendNext = 0;
                    data->lastPacketSent = 0;
                    socket.unsetFrom(data->sendSet);
                    return;
                } else {
                    data->incompleteSend = true;
                }
            } else {
                if (data->incompleteSend) {
                    if (chrono::steady_clock::now() - data->lastSuccessfulSendTime > sendTimeout) {
                        close();
                    }
                } else {
                    data->incompleteSend = true;
                }
            }
        } catch (exception &e) {

#ifdef LOGGING
            cout << "Tunnel socket flush failed with error: \n   what(): " << e.what() << endl;
#endif
            close();
        }
    }

    vector<CompressedIPPacket> readPackets(unsigned int max = numeric_limits<unsigned int>::max()) {
        vector<CompressedIPPacket> result{};
        if (!isOpen())return result;

        bool wrappedAround = false;
        unsigned int dispatchedStart;
        if (data->receiveNext == data->receiveDispatched) {
            dispatchedStart = 0;
            data->receiveNext = 0;
            data->receiveDispatched = 0;
            data->receiveRemaining = 0;
        } else {
            dispatchedStart = data->receiveDispatched;
        }

        while (true) {
            switch (data->receiveType) {
                case 0: {
                    auto &sock = getTCPSocket();
                    unsigned int currentBufferSize = wrappedAround ? dispatchedStart - data->receiveNext : data->size -
                                                                                                           data->receiveNext;

                    if (currentBufferSize < 1) {
                        if (wrappedAround)return result;
                        wrappedAround = true;
                        data->receiveNext = 0;
                        if (dispatchedStart < 1)return result;
                        currentBufferSize = dispatchedStart;
                    }
                    try {
                        auto res = sock.receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext, 1);
                        if (res > 0) {
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            data->receiveNext++;
                            char t = data->receiveBuffer[data->receiveDispatched++];
                            data->incompleteReceive = true;
                            data->receiveType = t;
                            unsigned int headerSize{};
                            if (t == DATA_SEGMENT)headerSize = sizeof(DataSegment);
                            else if (t == NEW_TUNNEL_RESPONSE_SEGMENT)
                                headerSize = sizeof(NewTunnelResponseSegment) + 16;
                            else if (t == DELETE_TUNNEL_SEGMENT)
                                headerSize = sizeof(DeleteTunnelSegment);

                            if (currentBufferSize < headerSize) {
                                if (wrappedAround)return result;
                                wrappedAround = true;
                                data->receiveNext = 0;
                                if (dispatchedStart < headerSize)return result;
                            }
                        } else return result;
                    } catch (exception &e) {
#ifdef LOGGING
                        cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                        close();
                        return result;
                    }
                    break;
                }
                case PING_SEGMENT: {
                    data->sendBuffer[data->sendNext] = PING_RESPONSE_SEGMENT;
                    if (data->size - data->sendNext > 1)data->sendNext++;
                    data->receiveType = 0;
                    data->incompleteReceive = false;
                    break;
                }
                case PING_RESPONSE_SEGMENT: {
                    data->hasSentPing = false;
                    data->receiveType = 0;
                    data->incompleteReceive = false;
                    break;
                }
                case DATA_SEGMENT: {
                    auto alreadyRead = data->receiveNext - data->receiveDispatched;
                    if (data->receiveRemaining == 0) {
                        try {
                            auto remaining = sizeof(DataSegment) - alreadyRead;
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) remaining);
                            if (res > 0) {
                                data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                                data->receiveNext += res;
                                if (res == remaining) {
                                    auto size = toHostByteOrder(((DataSegment *) (data->receiveBuffer +
                                                                                  data->receiveDispatched))->size);
                                    if (isValidPacketSize(size)) {
                                        unsigned int currentBufferSize = wrappedAround ? dispatchedStart -
                                                                                         data->receiveNext :
                                                                         data->size -
                                                                         data->receiveNext;
                                        if (currentBufferSize < size) {
                                            if (wrappedAround)return result;
                                            wrappedAround = true;
                                            data->receiveNext = 0;
                                            if (dispatchedStart < size)return result;
                                        }
                                        data->receiveRemaining = size;
                                    } else {
                                        close();
                                        return result;
                                    }
                                } else
                                    return result;
                            }
                        } catch (exception &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    } else {
                        try {
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) data->receiveRemaining);
                            if (res > 0) {
                                data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                                data->receiveNext += res;
                                if (res == data->receiveRemaining) {
                                    result.emplace_back(data->receiveBuffer + data->receiveDispatched,
                                                        data->receiveNext - data->receiveDispatched);
                                    data->receiveType = 0;
                                    data->receiveDispatched = data->receiveNext;
                                    data->receiveRemaining = 0;
                                    data->incompleteReceive = false;
                                    if (result.size() == max)return result;
                                } else {
                                    data->receiveRemaining -= res;
                                    return result;
                                }
                            }
                        } catch (exception &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    }
                    break;
                }
                case NEW_TUNNEL_REQUEST_SEGMENT: {
                    data->onNewTunnelRequest();
                    data->receiveType = 0;
                    data->incompleteReceive = false;
                    break;
                }
                case NEW_TUNNEL_RESPONSE_SEGMENT: {
                    auto alreadyRead = data->receiveNext - data->receiveDispatched;
                    if (alreadyRead < sizeof(NewTunnelResponseSegment)) {
                        try {
                            auto remaining = sizeof(NewTunnelResponseSegment) - alreadyRead;
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) remaining);
                            if (res > 0) {
                                data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                                data->receiveNext += res;
                                if (res == remaining) {
                                    auto family = ((NewTunnelResponseSegment *) (data->receiveBuffer +
                                                                                 data->receiveDispatched))->family;
                                    if (family == AF_INET)
                                        data->receiveRemaining = 4;
                                    else if (family == AF_INET6)
                                        data->receiveRemaining = 16;
                                    else {
                                        close();
                                        return result;
                                    }
                                } else
                                    return result;
                            }
                        } catch (exception &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    } else {
                        try {
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) data->receiveRemaining);
                            if (res > 0) {
                                data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                                data->receiveNext += res;
                                if (res == data->receiveRemaining) {
                                    /*IMP: the tunnel should validate the address*/
                                    auto &header = *(NewTunnelResponseSegment *) (data->receiveBuffer +
                                                                                  data->receiveDispatched);
                                    sockaddr_storage address{};
                                    if (header.family == AF_INET) {
                                        auto ain = reinterpret_cast<sockaddr_in *>(&address);
                                        ain->sin_port = header.port;
                                        memcpy(&ain->sin_addr, data->receiveBuffer + data->receiveDispatched +
                                                               sizeof(NewTunnelResponseSegment), 4);
                                    } else {
                                        /*Since we know it is ipv6 for sure, if not ipv4*/
                                        auto ain = reinterpret_cast<sockaddr_in6 *>(&address);
                                        ain->sin6_port = header.port;
                                        memcpy(&ain->sin6_addr, data->receiveBuffer + data->receiveDispatched +
                                                                sizeof(NewTunnelResponseSegment), 16);
                                    }
                                    data->onNewTunnelResponse(address);

                                    data->receiveType = 0;
                                    data->receiveDispatched = data->receiveNext;
                                    data->receiveRemaining = 0;
                                    data->incompleteReceive = false;
                                } else {
                                    data->receiveRemaining -= res;
                                    return result;
                                }
                            }
                        } catch (exception &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    }
                    break;
                }
                case DELETE_TUNNEL_SEGMENT: {
                    auto alreadyRead = data->receiveNext - data->receiveDispatched;
                    try {
                        auto remaining = sizeof(DeleteTunnelSegment) - alreadyRead;
                        auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                          (int) remaining);
                        if (res > 0) {
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            data->receiveNext += res;
                            if (res == remaining) {
                                auto &header = *reinterpret_cast<DeleteTunnelSegment *>(data->receiveBuffer +
                                                                                        data->receiveDispatched);
                                data->onDeleteTunnelRequest(toHostByteOrder(header.id));

                                data->receiveType = 0;
                                data->receiveDispatched = data->receiveNext;
                                data->incompleteReceive = false;
                            } else return result;
                        }
                    } catch (exception &e) {
#ifdef LOGGING
                        cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                        close();
                        return result;
                    }
                    break;
                }
                default: {
#ifdef LOGGING
                    cout << "Encountered odd segment type: " << data->receiveType << endl;
#endif
                    close();
                    return result;
                }
            }
        }
    }

    bool readPacket(CompressedIPPacket &packet)

    override {
        auto res = readPackets(1);
        if (res.

                empty()

                )
            return false;
        packet = res.at(0);
        return true;
    }

    bool isOpen() {
        return data->isOpen;
    }

    void close() {
        if (!isOpen()) {
            data->isOpen = false;
            socket.unsetFrom(data->receiveSet);
            socket.unsetFrom(data->sendSet);
            socket.unsetFrom(data->errorSet);
            socket.close();
        }
    }

    void checkStatus() {
        if (!isOpen())return;
        if (data->incompleteSend && chrono::steady_clock::now() - data->lastSuccessfulSendTime > sendTimeout) {
            close();
            return;
        }
        if (data->hasSentPing) {
#ifdef LOGGING
            cout << "Tunnel ping timed out" << endl;
#endif
            if (chrono::steady_clock::now() - data->lastPingTime > pingTimeout)close();
        } else {
            /*send ping*/
            if (data->size - data->sendNext > 0) {
                data->sendBuffer[data->sendNext++] = PING_SEGMENT;
            }
        }
    }

    bool sendNewTunnelRequest() {
        if (!isOpen() || sendAvailable() < 1)return false;
        data->sendBuffer[data->sendNext++] = NEW_TUNNEL_REQUEST_SEGMENT;
        return true;
    }

    bool sendNewTunnelResponse(sockaddr_storage &addr) {
        if (!isOpen())return false;
        if (addr.ss_family == AF_INET) {
            if (sendAvailable() < sizeof(NewTunnelResponseSegment) + 4 + 1)return false;
            data->sendBuffer[data->sendNext++] = NEW_TUNNEL_RESPONSE_SEGMENT;

            auto addrIn = reinterpret_cast<sockaddr_in *>(&addr);
            NewTunnelResponseSegment response{addrIn->sin_port, addr.ss_family};
            memcpy(data->sendBuffer + data->sendNext, &response, sizeof(NewTunnelResponseSegment));
            data->sendNext += sizeof(NewTunnelResponseSegment);

            memcpy(data->sendBuffer + data->sendNext, &addrIn->sin_addr, 4);
            data->sendNext += 4;
        } else if (addr.ss_family == AF_INET6) {
            if (sendAvailable() < sizeof(NewTunnelResponseSegment) + 16 + 1)return false;
            data->sendBuffer[data->sendNext++] = NEW_TUNNEL_RESPONSE_SEGMENT;

            auto addrIn6 = reinterpret_cast<sockaddr_in6 *>(&addr);
            NewTunnelResponseSegment response{addrIn6->sin6_port, addr.ss_family};
            memcpy(data->sendBuffer + data->sendNext, &response, sizeof(NewTunnelResponseSegment));
            data->sendNext += sizeof(NewTunnelResponseSegment);

            memcpy(data->sendBuffer + data->sendNext, &addrIn6->sin6_addr, 16);
            data->sendNext += 16;
        } else {
            throw invalid_argument("Unknown address family");
        }
        return true;
    }

    bool sendDeleteTunnelRequest(unsigned short id) {
        if (sendAvailable() < sizeof(DeleteTunnelSegment) + 4)return false;
        DeleteTunnelSegment segment{toNetworkByteOrder(id)};
        data->sendBuffer[data->sendNext++] = DELETE_TUNNEL_SEGMENT;
        memcpy(data->sendBuffer + data->sendNext, &segment, sizeof(DeleteTunnelSegment));
        data->sendNext += sizeof(DeleteTunnelSegment);
        return true;
    }

    void setId(unsigned id) {
        data->id = id;
    }

    unsigned int getId() {
        return data->id;
    }

    OnNewTunnelResponse &getOnNewTunnelResponse() {
        return data->onNewTunnelResponse;
    }

    void setOnNewTunnelResponse(OnNewTunnelResponse callback) {
        data->onNewTunnelResponse = std::move(callback);
    }

    OnNewTunnelRequest &getOnNewTunnelRequest() {
        return data->onNewTunnelRequest;
    }

    void setOnNewTunnelRequest(OnNewTunnelRequest callback) {
        data->onNewTunnelRequest = callback;
    }

    OnDeleteTunnelRequest &getOnDeleteTunnelRequest() {
        return data->onDeleteTunnelRequest;
    }

    void setOnDeleteTunnelRequest(OnDeleteTunnelRequest callback) {
        data->onDeleteTunnelRequest = std::move(callback);
    }

    void setOnUnsentData(OnUnsentData callback) {
        data->onUnsentData = callback;
    }

    OnUnsentData &getOnUnsentData() {
        return data->onUnsentData;
    }

    inline unsigned int sendAvailable() {
        return data->size - data->sendNext - 1; // The '1' is reserved for pinging back
    }

    inline bool receiveAvailable() {
        return data->size - data->receiveNext;
    }

private:

    struct Data {
        unsigned int size;
        char *sendBuffer = new char[size]{};
        unsigned int written{};
        unsigned int lastPacketSent{};
        unsigned int sendNext{};
        bool incompleteSend{};
        chrono::steady_clock::time_point lastSuccessfulSendTime{};//todo make accordingly

        char *receiveBuffer = new char[size]{};
        unsigned int receiveNext{};
        unsigned int receiveDispatched{};
        char receiveType{};
        unsigned int receiveRemaining{};
        bool incompleteReceive{};
        chrono::steady_clock::time_point lastSuccessfulReceiveTime{};

        OnNewTunnelResponse onNewTunnelResponse{};
        OnNewTunnelRequest onNewTunnelRequest{};
        OnDeleteTunnelRequest onDeleteTunnelRequest{};
        OnUnsentData onUnsentData{};


        fd_set &receiveSet;
        fd_set &sendSet;
        fd_set &errorSet;

        unsigned int id{};
        bool isOpen{true};

        bool hasSentPing{};
        chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastPingTime{};

        ~Data() {
            delete[] sendBuffer;
            delete[] receiveBuffer;
        }

        Data(unsigned int size, fd_set &rcv, fd_set &snd, fd_set &err) : size(size), receiveSet(rcv), sendSet(snd),
                                                                         errorSet(err) {

        }
    };

    shared_ptr<Data> data;
    constexpr static chrono::milliseconds pingTimeout{2000};
    constexpr static chrono::milliseconds sendTimeout{2000};
    constexpr static chrono::milliseconds receiveTimeout{2000};

    void advanceLastPacketSent() {
        while (data->lastPacketSent != data->written)
            switch (data->sendBuffer[data->lastPacketSent]) {
                case DATA_SEGMENT: {
                    auto &header = *(DataSegment *) (data->sendBuffer + data->lastPacketSent + 1);
                    auto totalSize = 1 + sizeof(DataSegment) + toHostByteOrder(header.size);
                    if (data->written - data->lastPacketSent < totalSize)return;
                    data->lastPacketSent += totalSize;
                    break;
                }
                case PING_SEGMENT:
                case PING_RESPONSE_SEGMENT:
                case NEW_TUNNEL_REQUEST_SEGMENT: {
                    data->lastPacketSent += 1;
                    break;
                }
                case NEW_TUNNEL_RESPONSE_SEGMENT: {
                    auto &header = *(NewTunnelResponseSegment *) (data->sendBuffer +
                                                                  data->lastPacketSent + 1);
                    auto totalSize = 1 + sizeof(NewTunnelResponseSegment) + (header.family == AF_INET ? 4 : 16);
                    if (data->written - data->lastPacketSent < totalSize)return;
                    data->lastPacketSent += totalSize;
                    break;
                }
                case DELETE_TUNNEL_SEGMENT: {
                    auto totalSize = 1 + sizeof(DeleteTunnelSegment);
                    if (data->written - data->lastPacketSent < totalSize)return;
                    data->lastPacketSent += totalSize;
                    break;
                }
                default: {
                    throw logic_error("Segment not implemented");
                }
            }
    }

    TCPSocket &getTCPSocket() {
        return *reinterpret_cast<TCPSocket *>(&socket);
    }

    inline void receivedPing() {

    }

};

#endif //TUNSERVER_DISPOSABLETUNNEL_H

#pragma clang diagnostic pop