//
// Created by yoni_ash on 4/27/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_TCPCONNECTION_H
#define TUNSERVER_TCPCONNECTION_H

#pragma once

#include "../Include.h"
#include "../packet/TCPPacket.h"
#include "../tunnel/Tunnel.h"

using namespace std;

#define RECEIVE_BUFFER_SIZE  (16*1024) //should be less than PACKET_SIZE - 60
#define SEND_WINDOW_SCALE 2
#define ACKNOWLEDGE_DELAY 250
#define SEND_MAX_RETRIES 3

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


    TCPConnection(Tunnel &tunnel, int &maxFd,
                  fd_set *rcv, fd_set
                  *snd,
                  fd_set *err
    );

    ~TCPConnection();

    inline states getState();

    inline int getFd() const;

    inline bool canHandle(TCPPacket &packet);

    void receiveFromClient(TCPPacket &);

    void receiveFromServer(TCPPacket &);

    void flushDataToServer(TCPPacket &);

    void flushDataToClient(TCPPacket &);

private:
    states state = CLOSED;
    bool clientReadFinished = false;
    bool serverReadFinished = false;
    int fd{};
    int &maxFd;

    Tunnel &tunnel;
    mutex mtx;
    sockaddr_in source{};
    sockaddr_in destination{};

    unsigned short mss{};
    unsigned char *sendBuffer{}; //buffer to send data to a client
    unsigned int sendLength{}; //size of buffer
    unsigned int sendWindow{}; // max size of data for next segment
    unsigned char windowShift{};
    unsigned int sendSequence{};
    unsigned int sendUnacknowledged{}; //the sequence number of next octet/data to send to the client
    unsigned int sendNext{}; //the sequence number of new data to send
    unsigned int sendNewDataSequence{};
    unsigned int retryCount{};
    chrono::duration<long, ratio<1, 1000000000>> rtt{};
    chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastSendTime{};
    chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastTimeAcknowledgmentAccepted{};

    unsigned int lastAcknowledgedSequence{};
    chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastAcknowledgmentSent{};
    unsigned char receiveBuffer[RECEIVE_BUFFER_SIZE]{}; //buffer to collect data from a client
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

    inline bool canSendToServer() const;

    inline bool canReceiveFromServer() const;

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
    ::close(fd);
    state = CLOSED;
}

unsigned int TCPConnection::getReceiveAvailable() const {
    return RECEIVE_BUFFER_SIZE - receiveNext + receiveSequence;
}

void TCPConnection::acknowledgeDelayed(TCPPacket &packet) {
    if (isUpStreamComplete())return;
    auto now = chrono::steady_clock::now();
    if ((chrono::duration_cast<chrono::milliseconds>(now - lastAcknowledgmentSent).count() &&
         lastAcknowledgedSequence != receiveNext) > ACKNOWLEDGE_DELAY ||
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

bool TCPConnection::canSendToServer() const {
//    char buf;
//    size_t res = send(fd, &buf, 0, 0);
//    return res == 0;
    return true;
}

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

bool TCPConnection::canReceiveFromServer() const {
//    char buf;
//    size_t res = read(fd, &buf, 0);
//    return res == 0;
    return true;
}

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
    auto amt = sendUnacknowledged - sendSequence;
    if (amt < mss)return;
    shiftElements(sendBuffer + amt, sendNewDataSequence - sendUnacknowledged, -(int) amt);
    sendSequence = sendUnacknowledged;
}

bool TCPConnection::canHandle(TCPPacket &packet) {
    return packet.isFrom(source);
}

int TCPConnection::getFd() const {
    return fd;
}


#endif //TUNSERVER_TCPCONNECTION_H

#pragma clang diagnostic pop