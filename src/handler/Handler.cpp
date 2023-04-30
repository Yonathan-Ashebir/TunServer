//
// Created by yoni_ash on 4/29/23.
//

#include "../Include.h"

#define PACKET_SIZE 1024

#include "Handler.h"

using namespace std;

void Handler::handleUpStream() {
    TCPPacket packet(PACKET_SIZE);
    fd_set rcv;
    fd_set snd;
    fd_set err;

    FD_ZERO(&rcv);
    FD_ZERO(&snd);
    FD_ZERO(&err);
    int tunnelFd = tunnel.getFileDescriptor();
    int maxFd = tunnelFd + 1;
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    FD_SET(tunnelFd, &rcv);
    while (shouldRun) {
        fd_set copy = rcv;
        timeval tvCpy = tv;
        int count = select(maxFd, &copy, nullptr, nullptr, &tvCpy);
        if (count < 0)exitWithError("Could not use select on tunnel's file descriptor");

        if (count && tunnel.readPacket(packet)) {
            auto client = packet.getSource();
            bool isSyn = packet.isSyn();
            bool isHandled = false;
            TCPConnection *closedConnection = nullptr;

            for (unsigned int ind = 0; ind < connectionsCount; ind++) {
                auto con = connections[ind];
                if (con->getState() == TCPConnection::CLOSED) {
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
                    if (connectionsCount == connectionsSize - 1) {
                        connectionsSize *= 2;
                        connections = (TCPConnection **) ::realloc(connections, connectionsSize);
                    }
                    if (closedConnection == nullptr) {
                        auto con = new TCPConnection(tunnel, maxFd, &rcv, &snd, &err);
                        connections[count] = con;
                        count++;
                        con->receiveFromClient(packet);
                    }
                } else {
                    packet.swapEnds();
                    packet.clearData();
                    packet.makeResetSeq(packet.getAcknowledgmentNumber());
                    tunnel.writePacket(packet);
                }
            }

        }
        //flush to server
        for (unsigned int ind = 0; ind < connectionsCount; ind++) {
            auto con = connections[ind];
            if (con->getState() == TCPConnection::CLOSED)continue;
            con->flushDataToServer(packet);
        }
    }
    upStreamShuttingDown = false;
    if (downStreamShuttingDown) return;
    else cleanUp();
}

void Handler::handleDownStream() {
    TCPPacket packet(PACKET_SIZE);
    fd_set rcv;
    fd_set snd;
    fd_set err;
    int maxFd = 0;

    FD_ZERO(&rcv);
    FD_ZERO(&snd);
    FD_ZERO(&err);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    int tunnelFd = tunnel.getFileDescriptor();
    while (shouldRun) {
        fd_set rcvCpy = rcv;
        timeval tvCpy = tv;
        int count = select(maxFd, &rcvCpy, nullptr, nullptr, &tvCpy);
        if (count < 0)exitWithError("Could not use select on tunnel's file descriptor");
        for (unsigned int ind = 0; ind < connectionsCount && count > 0; ind++) {
            auto con = connections[ind];
            if (con->getState() == TCPConnection::CLOSED)continue;
            if (FD_ISSET(con->getFd(), &rcvCpy)) {
                con->receiveFromServer(packet);
                count--;
            }
        }

        for (unsigned int ind = 0; ind < connectionsCount && count > 0; ind++) {
            auto con = connections[ind];
            if (con->getState() == TCPConnection::CLOSED)continue;
            con->flushDataToClient(packet);
        }
    }
    unique_lock<mutex>(mtx);
    downStreamShuttingDown = false;
    if (upStreamShuttingDown) return;
    else cleanUp();

}

bool Handler::start() {
    unique_lock<mutex>(mtx);
    if (shouldRun)return true;
    if (upStreamShuttingDown || downStreamShuttingDown)return false;
    shouldRun = true;
    thread th1{[this] { handleUpStream(); }};
    thread th2{[this] { handleDownStream(); }};
    return true;
}

bool Handler::stop() {
    shouldRun = false;
}

void Handler::cleanUp() {
    for (unsigned int ind = 0; ind < connectionsCount; ind++) {
        auto con = connections[ind];
        delete con;
    }
    delete connections;//todo: might collide with realloc
}

