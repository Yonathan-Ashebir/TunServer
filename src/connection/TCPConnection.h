//
// Created by yoni_ash on 4/27/23.
//

#ifndef TUNSERVER_TCPCONNECTION_H
#define TUNSERVER_TCPCONNECTION_H
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#pragma once

#include "../Include.h"
#include "../packet/TCPPacket.h"
#include "../tunnel/Tunnel.h"

using namespace std;

#define RECEIVE_BUFFER_SIZE  (16*1024) //should be less than PACKET_SIZE - 60
#define SEND_WINDOW_SCALE 2
#define ACKNOWLEDGE_DELAY 250
#define SEND_MAX_RETRIES 3

constexpr chrono::nanoseconds MAX_RTT(5000000000);

class TCPConnection {

public:
    enum states {
        CLOSED,
        SYN_SENT,
        SYN_RECEIVED,
        SYN_ACK_SENT,
        ESTABLISHED,
        TIME_WAIT,
    };


    TCPConnection(Tunnel &tunnel, socket_t &maxFd,
                  fd_set *rcv, fd_set
                  *snd,
                  fd_set *err
    );

    ~TCPConnection();

    inline states getState();

    inline socket_t getFd() const;

    inline bool canHandle(TCPPacket &packet);

    void receiveFromClient(TCPPacket &);

    bool receiveFromServer(TCPPacket &packet);

    void flushDataToServer(TCPPacket &);

    void flushDataToClient(TCPPacket &);

private:
    states state = CLOSED;
    bool clientReadFinished = false;
    bool serverReadFinished = false;
    socket_t fd{};
    socket_t &maxFd;

    Tunnel &tunnel;
    mutex mtx;
    sockaddr_in source{};
    sockaddr_in destination{};

    unsigned short mss{};
    BUFFER_BYTE *sendBuffer{}; //buffer to send data to a client
    unsigned int sendLength{}; //size of buffer
    unsigned int sendWindow{}; // max size of data for next segment
    unsigned char windowShift{};
    unsigned int sendSequence{};
    unsigned int sendUnacknowledged{}; //the sequence number of next octet/data to send to the client
    unsigned int sendNext{}; //the sequence number of new data to send
    unsigned int sendNewDataSequence{};
    bool sendBufferHasSpace = true;
    unsigned int retryCount{};
    chrono::duration<long, ratio<1, 1000000000>> rtt{};
    chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastSendTime{};
    chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastTimeAcknowledgmentAccepted{};

    unsigned int lastAcknowledgedSequence{};
    BUFFER_BYTE receiveBuffer[RECEIVE_BUFFER_SIZE]{}; //buffer to collect data from a client
    unsigned int receiveSequence{};
    unsigned int receiveUser{}; //the sequence number of next data
    // to be consumed by the user or in our case sends to the application server
    unsigned int receiveNext{}; //the sequence number of next expected data
    unsigned int receivePushSequence{};
    fd_set *receiveSet{};
    fd_set *sendSet{};
    fd_set *errorSet{};

    inline void closeConnection();

    inline unsigned int getReceiveAvailable() const;

    inline unsigned int getSendAvailable() const;

    inline bool isUpStreamComplete() const;

    inline bool isDownStreamComplete() const;

    inline void _trimReceiveBuffer();

    inline void trimReceiveBuffer();

    inline void trimSendBuffer();

    inline void acknowledgeDelayed(TCPPacket &packet);
};

TCPConnection::states TCPConnection::getState() {
    return state;
}

void TCPConnection::closeConnection() {
    //todo: might affect opposite stream
    FD_CLR(fd, receiveSet);
    FD_CLR(fd, sendSet);
    FD_CLR(fd, errorSet);
    CLOSE(fd);
    state = CLOSED;
}

unsigned int TCPConnection::getReceiveAvailable() const {
    return RECEIVE_BUFFER_SIZE - receiveNext + receiveSequence;
}

void TCPConnection::acknowledgeDelayed(TCPPacket &packet) {
    if (isUpStreamComplete())return;
    static chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastAcknowledgmentSent{};
    auto now = chrono::steady_clock::now();
    if ((chrono::duration_cast<chrono::milliseconds>(now - lastAcknowledgmentSent).count() > ACKNOWLEDGE_DELAY &&
         lastAcknowledgedSequence != receiveNext) ||
        receiveNext - lastAcknowledgedSequence > mss * 2) {
        packet.setEnds(destination, source);
        packet.clearData();
        packet.makeNormal(sendNext, receiveNext);
        tunnel.writePacket(packet);
        lastAcknowledgmentSent = now;
        lastAcknowledgedSequence = receiveNext;
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"


#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"


#pragma clang diagnostic pop

unsigned int TCPConnection::getSendAvailable() const {
    return sendLength - sendNewDataSequence + sendSequence;
}

bool TCPConnection::isUpStreamComplete() const {
    return clientReadFinished && receiveUser >= receiveNext - 1;
}

bool TCPConnection::isDownStreamComplete() const {
    return serverReadFinished && sendUnacknowledged == sendNewDataSequence;
}

void TCPConnection::trimReceiveBuffer() {
    if (receiveUser - receiveSequence < mss)return;
    _trimReceiveBuffer();
}

void TCPConnection::_trimReceiveBuffer() {
    if (receiveUser == receiveSequence)return;
    unsigned int amt = receiveUser - receiveSequence;
    shiftElements(receiveBuffer + amt, receiveNext - receiveUser, -(int) amt);
    receiveSequence = receiveNext;
}

void TCPConnection::trimSendBuffer() {
    auto space = sendUnacknowledged - sendSequence;
    if (space < mss || getSendAvailable() > mss * 2)return;
#ifdef  LOGGING
    ::printf("Trimmed send buffer by %d\n", space);
#endif
    shiftElements(sendBuffer + space, sendNewDataSequence - sendUnacknowledged, -(int) space);
    sendSequence = sendUnacknowledged;
}

bool TCPConnection::canHandle(TCPPacket &packet) {
    return packet.isFrom(source);
}

socket_t TCPConnection::getFd() const {
    return fd;
}


#endif //TUNSERVER_TCPCONNECTION_H

#pragma clang diagnostic pop