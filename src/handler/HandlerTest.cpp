//
// Created by yoni_ash on 5/12/23.
//
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <csignal>
#include <condition_variable>
#include "../Include.h"
#include "./Handler.h"
#include <tun-commons/tunnel/DatagramTunnel.h>

using namespace std;

#define PACKET_SIZE 3072;
#define IP_ADDR "0.0.0.0"

using namespace std;

void handleSingleConnection() {
    initPlatform();

    UDPSocket tunnelSocket;
    tunnelSocket.setReuseAddress(true);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDR, &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);
    tunnelSocket.bind(addr);

    ::printf("Waiting for first packet\n");

    char buf[3072];
    sockaddr_in from{};
    auto r = tunnelSocket.receiveFrom(buf, sizeof buf, from);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
    printf("Received %d bytes from %s:%d\n", r, ip, ntohs(from.sin_port));
    tunnelSocket.connect(from);

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    DatagramTunnel tunnel(tunnelSocket);
    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;
    socket_t maxFd{};
    TCPPacket packet(3072);
    ::printf("Waiting for a syn\n");
    do {
        tunnel.readPacket(packet);
    } while (!packet.isSyn());
    ::printf("First Syn received\n");
    TCPSession connection(tunnel, maxFd, readSet, writeSet, errorSet);
    connection.receiveFromClient(packet);

    auto t1 = thread{[&tunnel, &connection] {
        TCPPacket packet(3072);
        while (true) {
            tunnel.readPacket(packet);
            if (connection.getState() == TCPSession::CLOSED || connection.canHandle(packet)) {
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

void handleDownload() {
    initPlatform();

    UDPSocket tunnelSocket;
    tunnelSocket.setReuseAddress(true);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDR, &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);
    tunnelSocket.bind(addr);

    ::printf("Waiting for first firstSynPacket\n");

    char starterBuffer[3072];
    socklen_t socklen = sizeof(sockaddr_in);
    sockaddr_in from{};
    auto r = tunnelSocket.receiveFrom(starterBuffer, sizeof starterBuffer, from);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
    printf("Received %d bytes from %s:%d and addrLen: %d\n", r, ip, ntohs(from.sin_port), socklen);

    tunnelSocket.connect(from);


#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    DatagramTunnel tunnel(tunnelSocket);
    TCPPacket firstSynPacket(3072);
    do {
        tunnel.readPacket(firstSynPacket);
    } while (!firstSynPacket.isSyn());
    ::printf("First Syn received\n");


    sockaddr_in server = firstSynPacket.getDestination();
    sockaddr_in client = firstSynPacket.getSource();
    inet_ntop(AF_INET, &(server.sin_addr.s_addr), ip, sizeof ip);
    printf("Forwarding to %s:%d\n", ip, ntohs(from.sin_port));
    bool isOpen = true;
    unsigned retryCount = 0;

    unsigned receiveNext = firstSynPacket.getSequenceNumber() + 1;

    unsigned int sendLength = 65535;
    char sendBuffer[sendLength];
    unsigned sendSequence = 1;
    unsigned sendUnacknowledged = sendSequence, sendNext = sendSequence, sendNewData = sendSequence;
    bool serverReadFinished = false;
    bool clientReadFinished = false;
    unsigned short mss = firstSynPacket.getMSS();

    firstSynPacket.swapEnds();
    firstSynPacket.makeNormal(sendUnacknowledged, receiveNext);
    firstSynPacket.makeSyn(sendUnacknowledged, receiveNext);
    //No need to accept acknowledgement, the test is under reliable network session
    sendSequence++;
    sendUnacknowledged++;
    sendNext++;
    sendNewData++;

    tunnel.writePacket(firstSynPacket);

    TCPSocket sock;
    sock.connect(server);

    unsigned int fromClient{};
    unsigned int fromServer{};
    condition_variable condition{};
    mutex mtx{};
    thread receiveFromClient{[&] {
        TCPPacket receivingPacket(3072);
        char buf[3072];
        while (isOpen) {
            tunnel.readPacket(receivingPacket);
            //No regenerate demanding or other cases are required here
            if (receivingPacket.isFrom(client) && receivingPacket.isAck()) {
                unsigned ack = receivingPacket.getAcknowledgmentNumber();
                //Sending data directly
                auto len = receivingPacket.getDataLength();
                if (len > 0 && receivingPacket.getSequenceNumber() == receiveNext) {
                    receiveNext += len;
                    fromClient += len;
                    receivingPacket.copyDataTo(buf, sizeof buf);
                    auto total = sock.send(buf, len);
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
                    sock.shutdownWrite();
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
            this_thread::sleep_for(chrono::milliseconds (10));
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

        auto total = sock.tryReceive(sendBuffer + sendNewData - sendSequence, available);
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

    resendToClient.join();
    receiveFromClient.join();

}


int main() {
//    handleDownload();
handleSingleConnection();
    return 0;
}

#pragma clang diagnostic pop