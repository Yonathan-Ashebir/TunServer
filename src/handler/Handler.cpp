//
// Created by yoni_ash on 4/29/23.
//

#include "../Include.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#define PACKET_SIZE 3072

#include "Handler.h"

using namespace std;

void Handler::handleUpStream() {
    TCPPacket packet(PACKET_SIZE);

    auto tunnelSocket = tunnel.getSocket();
    socket_t maxFdTunnel = tunnelSocket.getFD() + 1;
    timeval tv{0, 10000};

    fd_set tunnelRcv{};
    FD_ZERO(&tunnelRcv);
    tunnelSocket.setIn(tunnelRcv);
#ifdef LOGGING
#if _WIN32
    ::printf("Handling up streams, tunnelSocket: %llu\n", tunnelSocket.getFD());
#else
    ::printf("Handling up streams, tunnelSocket: %d\n", tunnelSocket.getFD());
#endif
#endif
    while (shouldRun) {
        fd_set copy = tunnelRcv;
        timeval tvCpy = tv;

        int count = select(maxFdTunnel, &copy, nullptr, nullptr, // NOLINT(cppcoreguidelines-narrowing-conversions)
                           &tvCpy); // NOLINT(cppcoreguidelines-narrowing-conversions)

        if (count == -1)throw SocketException("Could not use select on tunnel's file descriptor");

        if (count > 0 && tunnel.readPacket(packet)) {
            char srcIp[INET_ADDRSTRLEN];
            char destIp[INET_ADDRSTRLEN];

            unsigned int addr = htonl(packet.getSourceIp());
            inet_ntop(AF_INET, &addr, srcIp, INET_ADDRSTRLEN);

            addr = htonl(packet.getDestinationIp());
            inet_ntop(AF_INET, &addr, destIp, INET_ADDRSTRLEN);

#ifdef LOGGING
            printf("Packet src: %s, dest: %s, proto: %d, len: %d, isSyn: %b, isFin: %b, isAck: %b, isRst: %b, isUrg: %b\n",
                   srcIp, destIp, packet.getProtocol(),
                   packet.getLength(), packet.isSyn(), packet.isFin(), packet.isAck(), packet.isReset(),
                   packet.isUrg());
#endif

            bool isSyn = packet.isSyn();
            bool isHandled = false;
            TCPSession *closedConnection = nullptr;

            for (auto con: connections) {
                if (con->getState() == TCPSession::CLOSED) {
                    if (closedConnection == nullptr)closedConnection = con;
                    continue;
                }
                if (con->canHandle(packet)) {
                    con->receiveFromClient(packet);
                    isHandled = true;
                }
            }
            if (!isHandled) {
                if (isSyn) {
                    if (closedConnection == nullptr) {
                        auto con = new TCPSession(tunnel, maxFd, rcv, snd, err);
                        connections.push_back(con);
                        con->receiveFromClient(packet);
                    } else closedConnection->receiveFromClient(packet);
                } else {
                    packet.swapEnds();
                    packet.clearData();
                    packet.makeResetSeq(packet.getAcknowledgmentNumber());
                    tunnel.writePacket(packet);
                }
            }

        }
        //flush to server
        for (auto con: connections) {
            if (con->getState() == TCPSession::CLOSED)continue;
            con->flushDataToServer(packet);
        }
    }
    upStreamShuttingDown = false;
    if (downStreamShuttingDown) return;
    else cleanUp();
    upStreamActive = false;
}

void Handler::handleDownStream() {
    TCPPacket packet(PACKET_SIZE);

    FD_ZERO(&rcv);
    FD_ZERO(&snd);
    FD_ZERO(&err);
    timeval tv{0, 10000};

#ifdef _WIN32
    //todo: find a better way to resolve windows can not select fd_set with zero sockets set
    TCPSocket testSock;
    testSock.setReuseAddress(true);
    testSock.setIn(rcv);
#endif

#ifdef LOGGING
    ::printf("Handling down streams\n");
#endif
    while (shouldRun) {
        fd_set rcvCpy = rcv;
        timeval tvCpy = tv;
        int count = select(maxFd, &rcvCpy, nullptr, nullptr, &tvCpy); // NOLINT(cppcoreguidelines-narrowing-conversions)

#ifdef _WIN32
        if (count == SOCKET_ERROR) {
            printError("Could not use select on one of the socket's file descriptor", WSAGetLastError());
            exit(1);
        }
#else
        if (count == -1)exitWithError("Could not use select on one of the socket's file descriptor");
#endif

        bool atleastOneHasSpace = false;
        for (unsigned int ind = 0; ind < connections.size() && count > 0; ind++) {
            auto con = connections[ind];
            if (con->getState() == TCPSession::CLOSED)continue;
            if (con->getSocket().isSetIn(rcv)) {
                atleastOneHasSpace |= con->receiveFromServer(packet);
                count--;
            }
        }

        for (auto con: connections) {
            if (con->getState() == TCPSession::CLOSED)continue;
            con->flushDataToClient(packet);
        }
        /*No need to check effect of flushing as things are immediate in networking*/
//        usleep(100);
        if (!atleastOneHasSpace)usleep(1000);
    }

    downStreamShuttingDown = false;
    if (upStreamShuttingDown) return;
    else cleanUp();
    downStreamActive = false;
}

bool Handler::start() {
    unique_lock<mutex> lock(mtx);
    if (shouldRun)return true;
    if (downStreamActive || upStreamActive)return false;
    shouldRun = downStreamActive = upStreamActive = true;
    thread th1{[this] { handleUpStream(); }};
    thread th2{[this] { handleDownStream(); }};

    th1.join();
    th2.join();
    return true;
}

void Handler::stop() {
    unique_lock<mutex> lock(mtx);
    if (!shouldRun)return;
    shouldRun = false;
    upStreamShuttingDown = true;
    downStreamShuttingDown = true;
}

void Handler::cleanUp() {
    for (auto con: connections) {
        delete con;
    }
}

Handler::Handler(Tunnel &tunnel) : tunnel(tunnel) {
}


#pragma clang diagnostic pop