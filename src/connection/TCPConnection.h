//
// Created by yoni_ash on 4/27/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_TCPCONNECTION_H
#define TUNSERVER_TCPCONNECTION_H

#include "../Include.h"
#include "../packet/TCPPacket.h"
#include "../tunnel/Tunnel.h"

using namespace std;

#define RECEIVE_BUFFER_SIZE  (unsigned int )200//should be less than PACKET_SIZE - 60
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


    TCPConnection(Tunnel &tunnel, fd_set *rcv, fd_set *snd, fd_set *err);

    states getState();

    void receiveFromClient(TCPPacket &);

    void receiveFromServer(TCPPacket &);

    inline void flushDataToServer(TCPPacket &);

    inline void flushDataToClient(TCPPacket &);

private:
    states state = CLOSED;
    bool clientReadFinished = false;
    bool serverReadFinished = false;
    int fd{};

    Tunnel &tunnel;
    mutex mtx;
    sockaddr_in source{};
    sockaddr_in destination{};

    unsigned short mss{};
    unsigned char *sendBuffer{}; //buffer to send data to client
    unsigned int sendLength{}; //size of buffer
    unsigned int sendWindow{}; // max size of data for next segment
    unsigned char windowShift{};
    unsigned int sendSequence{};
    unsigned int sendUnacknowledged{}; //the sequence number of next octet/data to send to the client
    unsigned int sendNext{}; //the sequence number of new data to send
    unsigned int sendNewDataSequence{};
    unsigned int retryCount{};
    chrono::duration<long, ratio<1, 1000000000>> rtt{};
    chrono::time_point<chrono::steady_clock, chrono::duration<long, ratio<1, 1000000000>>> lastSendTime{};
    chrono::time_point<chrono::steady_clock, chrono::duration<long, ratio<1, 1000000000>>> lastTimeAcknowledgmentAccepted{};

    unsigned int lastAcknowledgeSequence{};
    chrono::time_point<chrono::steady_clock, chrono::duration<long, ratio<1, 1000000000>>> lastAcknowledgmentSent{};
    unsigned char receiveBuffer[RECEIVE_BUFFER_SIZE]{}; //buffer to collect data from client
    unsigned int receiveSequence{};
    unsigned int receiveUser{}; //the sequence number of next data to be consumed by the user or in our case send to the application server
    unsigned int receiveNext{}; //the sequence number of next expected data

    fd_set *receiveSet{};
    fd_set *sendSet{};
    fd_set *errorSet{};

    inline void closeConnection();

    constexpr unsigned int getReceiveAvailable() const;

    constexpr unsigned int getSendAvailable() const;

    constexpr bool isUpStreamComplete() const;

    constexpr bool isDownStreamComplete() const;

    inline bool canSendToServer() const;

    inline bool canReceiveFromServer() const;

    inline void _trimReceiveBuffer();

    inline void trimReceiveBuffer();

    inline void trimSendBuffer();

    inline void acknowledgeDelayed(TCPPacket &packet);
};


#endif //TUNSERVER_TCPCONNECTION_H

#pragma clang diagnostic pop