//
// Created by yoni_ash on 5/12/23.
//
#include <csignal>
#include <condition_variable>
#include "../Include.h"
#include "./Handler.h"
#include "../tunnel/DatagramTunnel.h"

#define PACKET_SIZE 3072;
using namespace std;

void handleSingleConnection() {
    initPlatform();

    socket_t tunnelFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (tunnelFd < 0)exitWithError("Could not create a socket");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.71.64", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);
    auto b1 = bind(tunnelFd, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b1 < 0)exitWithError("Could not bind");

    ::printf("Waiting for first packet\n");

    char buf[3072];
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in from{};
    auto r = recvfrom(tunnelFd, buf, sizeof buf, 0, reinterpret_cast<sockaddr *>(&from), &len);
    if (r < 0)exitWithError("Could not receive from socket");

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
    printf("Received %zd bytes from %s:%d and addrLen: %d\n", r, ip, ntohs(from.sin_port), len);

    auto c = connect(tunnelFd, (sockaddr *) &from, len);
    if (c < 0)exitWithError("Could not connect socket");

//    if (fcntl(tunnelFd, F_SETFL, O_NONBLOCK) == -1) {
//        exitWithError("Could not set tunnel non-blocking");
//    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    DatagramTunnel tunnel(tunnelFd);
    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;
    int maxFd{};
    TCPPacket packet(3072);
    ::printf("Waiting for a syn\n");
    do {
        tunnel.readPacket(packet);
    } while (!packet.isSyn());
    ::printf("First Syn received\n");
    TCPConnection connection(tunnel, maxFd, &readSet, &writeSet, &errorSet);
    connection.receiveFromClient(packet);

    auto t1 = thread{[&tunnel, &connection] {
        TCPPacket packet(3072);
        while (true) {
            tunnel.readPacket(packet);
            if (connection.getState() == TCPConnection::CLOSED || connection.canHandle(packet)) {
                connection.receiveFromClient(packet);
            }
            connection.flushDataToServer(packet);
        }
    }};

    while (true) {
        timeval timeout{0, 10000};
        int count = select(maxFd, &readSet, nullptr, nullptr, &timeout);
        if (count > 0) {
            connection.receiveFromServer(packet);
        }
        connection.flushDataToClient(packet);
    }

}

socket_t _createTcpSocket() {
    socket_t result;
#ifdef _WIN32
    result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif

    int val = 1;
//    if (setsockopt(result, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
//        exitWithError("Could not disable Nagle algorithm");

    return result;
}

void handleDownload() {
    initPlatform();

    socket_t tunnelFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (tunnelFd < 0)exitWithError("Could not create a socket");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.238.64", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);
    auto b1 = bind(tunnelFd, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b1 < 0)exitWithError("Could not bind");

    ::printf("Waiting for first firstSynPacket\n");

    char starterBuffer[3072];
    socklen_t socklen = sizeof(sockaddr_in);
    sockaddr_in from{};
    auto r = recvfrom(tunnelFd, starterBuffer, sizeof starterBuffer, 0, reinterpret_cast<sockaddr *>(&from), &socklen);
    if (r < 0)exitWithError("Could not receive from socket");

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
    printf("Received %zd bytes from %s:%d and addrLen: %d\n", r, ip, ntohs(from.sin_port), socklen);

    auto c = connect(tunnelFd, (sockaddr *) &from, socklen);
    if (c < 0)exitWithError("Could not connect socket");

//    if (fcntl(tunnelFd, F_SETFL, O_NONBLOCK) == -1) {
//        exitWithError("Could not set tunnel non-blocking");
//    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    DatagramTunnel tunnel(tunnelFd);
    TCPPacket firstSynPacket(3072);
    do {
        tunnel.readPacket(firstSynPacket);
    } while (!firstSynPacket.isSyn());
    ::printf("First Syn received\n");


    sockaddr_in server = firstSynPacket.getDestination();
    sockaddr_in client = firstSynPacket.getSource();
    bool isOpen = true;
    unsigned retryCount = 0;

    unsigned receiveNext = firstSynPacket.getSequenceNumber() + 1;

    unsigned int sendLength = 65535;
    unsigned char sendBuffer[sendLength];
    unsigned sendSequence = 1;
    unsigned sendUnacknowledged = sendSequence, sendNext = sendSequence, sendNewData = sendSequence;
    bool serverReadFinished = false;
    bool clientReadFinished = false;
    unsigned short mss = firstSynPacket.getMSS();

    firstSynPacket.swapEnds();
    firstSynPacket.makeNormal(sendUnacknowledged, receiveNext);
    firstSynPacket.makeSyn(sendUnacknowledged, receiveNext);
    //No need to accept acknowledgement, the test is under reliable network connection
    sendSequence++;
    sendUnacknowledged++;
    sendNext++;
    sendNewData++;

    tunnel.writePacket(firstSynPacket);

    socket_t sock = _createTcpSocket();
    auto res = connect(sock, reinterpret_cast<const sockaddr *>(&server), sizeof(server));
    if (res != 0)exitWithError("Could not connect to server");

    unsigned int fromClient{};
    unsigned int fromServer{};
    condition_variable condition{};
    mutex mtx{};
    thread receiveFromClient{[&] {
        TCPPacket receivingPacket(3072);
        unsigned char buf[3072];
        while (isOpen) {
            tunnel.readPacket(receivingPacket);
            //No reset demanding or other cases are required here
            if (receivingPacket.isFrom(client) && receivingPacket.isAck()) {
                unsigned ack = receivingPacket.getAcknowledgmentNumber();
                //Sending data directly
                auto len = receivingPacket.getDataLength();
                if (len > 0 && receivingPacket.getSequenceNumber() == receiveNext) {
                    receiveNext += len;
                    fromClient += len;
                    receivingPacket.copyDataTo(buf, sizeof buf);
                    auto total = send(sock, buf, len, 0);
                    if (total != len)exitWithError("Could not send all data in the received packet");
                }

                retryCount = 0;
                unique_lock<mutex> lock(mtx);
                sendUnacknowledged = max(sendUnacknowledged, ack);
                if (sendUnacknowledged - sendSequence > mss) {
                    condition.notify_all();
                }
                lock.unlock();

                //It was possible not to use getSegmentLength
                receiveNext = max(receiveNext,
                                  receivingPacket.getSequenceNumber() + receivingPacket.getSegmentLength());
                if (receivingPacket.isFin() && !clientReadFinished) {
                    clientReadFinished = true;
                    shutdown(sock, SHUT_WR);
                }
                if (clientReadFinished && serverReadFinished && sendUnacknowledged == sendNewData)isOpen = false;
            }
        }
        ::printf("Receiving from the client is finished\n");
    }};

    constexpr chrono::milliseconds resendTimeOut(2000);
    thread resendToClient{[&] {
        TCPPacket resendingPacket(3072);
        resendingPacket.setEnds(server, client);
        resendingPacket.makeNormal(sendNext, receiveNext);
        auto lastResendTime = chrono::steady_clock::now();
        while (isOpen && (!serverReadFinished || sendUnacknowledged < sendNext)) {
            auto now = chrono::steady_clock::now();
            if (now - lastResendTime > resendTimeOut) {
                lastResendTime = now;
                if (sendUnacknowledged < sendNext) {
                    if (retryCount > 2) {
                        isOpen = false;
                        break;
                    }
                    retryCount++;
                    for (unsigned seq = sendUnacknowledged; seq < sendNext;) {
                        auto len = min(sendNext - seq - (serverReadFinished ? 1 : 0), (unsigned) mss);
                        resendingPacket.setSequenceNumber(seq);
                        resendingPacket.clearData();
                        resendingPacket.appendData(sendBuffer + seq - sendSequence, len);

                        if (serverReadFinished && seq == sendNext - 1) {
                            resendingPacket.setFinFlag(true);
                            seq++;
                        } else resendingPacket.setFinFlag(false);

                        tunnel.writePacket(resendingPacket);
                        seq += len;
                    }
                }
            }
            usleep(10000);
        }
        ::printf("Resend to the client is finished\n");
    }};

    TCPPacket sendingPacket(3072);
    sendingPacket.setEnds(server, client);
    sendingPacket.makeNormal(sendNext, receiveNext);
    while (isOpen && !serverReadFinished) {
        unsigned available = sendLength - sendNewData + sendSequence;
        if (available < mss * 2) {
            unique_lock<mutex> lock(mtx);
            auto reusable = sendUnacknowledged - sendSequence;
            if (available == 0 && reusable == 0) {
                condition.wait(lock);
            }
            lock.unlock();
            reusable = sendUnacknowledged - sendSequence;
            shiftElements(sendBuffer + reusable, sendNewData - sendUnacknowledged, -(int) reusable);
            available += reusable;
            sendSequence += reusable;
        }

        auto total = recv(sock, sendBuffer + sendNewData - sendSequence, available, 0);
        if (total > 0) {
            sendNewData += total;
            fromServer += total;
            if (sendNewData - sendNext > mss) {
                for (unsigned seq = sendNext; seq < sendNewData;) {
                    auto len = min(sendNewData - seq, (unsigned) mss);
                    sendingPacket.setSequenceNumber(seq);
                    sendingPacket.clearData();
                    sendingPacket.appendData(sendBuffer + seq - sendSequence, len);
                    tunnel.writePacket(sendingPacket);
                    seq += len;
                }
                sendNext = sendNewData;
            }
        } else if (total == 0) {
            sendNewData++;
            sendingPacket.clearData();
            sendingPacket.setSequenceNumber(sendNext++);
            //No need to set the fin flag to false as the loop is done
            sendingPacket.setFinFlag(true);
            tunnel.writePacket(sendingPacket);
            //Time to break the loop
            serverReadFinished = true;
        } else {//The socket is assumed to be blocking, so no EAGAIN or EWOULDBLOCK
            exitWithError("Error reading from server");
        }
    }

    ::printf("Receiving from the application server is completed\n");

    resendToClient.

            join();

    receiveFromClient.

            join();

    CLOSE(sock);
    CLOSE(tunnelFd);
}


int main() {
    handleDownload();
    return 0;
}