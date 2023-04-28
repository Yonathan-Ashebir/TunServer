//
// Created by yoni_ash on 4/15/23.
//

#include <mutex>
#include "HandleTun.h"
#include "packet/TCPPacket.h"
#include "tunnel/Tunnel.h"

using namespace std;

#define PACKET_SIZE 300
#define RECEIVE_BUFFER_SIZE  (unsigned int )200//should be less than PACKET_SIZE - 60
#define SEND_BUFFER_SIZE 200
#define  MAX_CONNECTIONS 200
const int IP_HEADER_SIZE = sizeof(iphdr);
const int MIN_HEADER_SIZE = IP_HEADER_SIZE + sizeof(tcphdr);

//info: used abbreviation
enum states {
    closed, synSent, synReceived, established, finWait1, finWait2, closing, timeWait, closeWait, lastAck
};

struct connection {
    states state = closed;
    int fd{};
    mutex mtx;
    sockaddr_in src{};
    sockaddr_in dest{};
    unsigned short mss{};
    unsigned char *sBuf{}; //buffer to send data to client
    unsigned int sndLen{}; //size of buffer
    unsigned int sndWnd{}; // max size of data for next segment
    unsigned int sndUna{}; //the sequence number of next octet/data to send to the client
    unsigned int sndNxt{}; //the sequence number of new data to send


    unsigned char rBuf[RECEIVE_BUFFER_SIZE]{}; //buffer to collect data from client
    unsigned int rcvNxt{}; //the sequence number of next expected data
    unsigned int rcvUsr{}; //the sequence number of next data to be consumed by the user or in our case send to the application server
    unsigned int rcvSeq{};
};


int createTcpSocket() {
    int result = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);//warn: might not work on windows
    return result;
}

//handleTun
connection *connections[MAX_CONNECTIONS];
int fdMin;
int fdMax;

inline void closeConnection(connection *con) {
    CLOSE(con->fd);
    con->state = closed;
}

void handleServersToClients(Tunnel &tunnel, connection *connections[], unsigned int length) {
    TCPPacket packet(PACKET_SIZE);

    fdMax = fdMin = tunnel.getFd();
    fd_set master;
    fd_set rcvSet;

    timeval timeout = {0, 10000};

    FD_ZERO(&master);
    FD_ZERO(&rcvSet); //warn: not sure if this is necessary

    FD_SET(fdMin, &master);
    while (true) {
        rcvSet = master;
        int count;
        if ((count = select(fdMax + 1, &rcvSet, nullptr, nullptr, &timeout)) == -1) {
            perror("Select failed at handleTun");
            exit(4);
        }
        for (int fd = fdMin; fd <= fdMax && count > 0; fd++) {
        }
    }
}


void handleClientsToServers(Tunnel tunnel, connection *connections[], unsigned int length) {
    //packet buffer for reusing
    auto createConnection = [&](int fd, sockaddr_in src, sockaddr_in dest, unsigned int seqNo, unsigned int ackNo,
                                unsigned int window,
                                unsigned short mss) {
        connection *res;
        if (fd > fdMax) {
            res = new connection;
            connections[fd - fdMin] = res;
            fdMax = fd;
        } else res = connections[fd - fdMin];

        res->fd = fd;
        res->state = synSent;
        res->src = src;
        res->dest = dest;
        res->mss = mss;

        res->rcvSeq = ackNo;
        res->rcvNxt = ackNo;

        res->sndUna = seqNo;
        res->sndNxt = seqNo + 1;
        res->sndWnd = window;

        u_int32_t sendLen = window * 2;
        if (sendLen > res->sndLen) {
            res->sndLen = sendLen;
            delete res->sBuf;
            res->sBuf = (unsigned char *) malloc(sendLen);
        }
    };
    auto generateSequenceNo = []() {
        return rand() % 2 ^ 31;
    };

    TCPPacket packet(PACKET_SIZE);
    fd_set master;
    fd_set rcvSet;
    timeval timeout = {0, 10000};

    FD_ZERO(&master);
    FD_ZERO(&rcvSet); //warn: not sure if this is necessary
    FD_SET(tunnel.getFd(), &master);

    while (true) {
        rcvSet = master;
        int count = select(fdMax + 1, &rcvSet, nullptr, nullptr, &timeout);

        if (count == -1) {
            perror("Select failed at handleTun");
            exit(4);
        }

        if (count > 0) {
            tunnel.readPacket(packet);
            sockaddr_in client = packet.getSource();
            sockaddr_in server = packet.getDestination(); //i.e application server

            if (packet.isSyn()) {
                bool success = false;
                if (packet.isAck()) {
                    printf("Accepting syn_ack is not supported\n");//todo: this is not right. It might just need ack, with current snd.nxt, rcv.nxt
                    goto forEnd;
                }
                for (int fd2 = fdMin; fd2 <= fdMax; fd2++) {
                    connection *con = connections[fd2 - fdMin];
                    if (packet.isFrom(con->src)) {
                        if (con->state != closed && con->rcvNxt - 1 != packet.getSequenceNumber()) {
                            closeConnection(con);
                            unsigned rstSeq = packet.getSequenceNumber() + packet.getDataLength();
                            packet.makeResetSeq(rstSeq);
                            packet.setEnds(server, client);
                            tunnel.writePacket(packet);
                            goto forEnd;
                        }
                    }
                }
                int sock = createTcpSocket();
                if (sock < 0) {
                    printf("Could not create socket");
                }
                if (sock >= 0) {
                    if (connect(sock, (sockaddr *) &server, sizeof(sockaddr_in)) == 0) {
                        unsigned int seq = generateSequenceNo();
                        unsigned int ackSeq = packet.getSequenceNumber() + 1;
                        unsigned int windowSize = packet.getWindowSize();
                        unsigned short mss = packet.getMSS();

                        createConnection(sock, client, server, seq, ackSeq, windowSize, mss);
                        packet.makeSyn(seq, ackSeq);
                        packet.setEnds(server, client);
                        tunnel.writePacket(packet);
                        success = true; //todo: might not be nessecary
                        continue;
                    }
                }
                unsigned rstSeq = packet.getSequenceNumber() + packet.getDataLength();
                packet.makeResetSeq(rstSeq);
                packet.setEnds(server, client);
                tunnel.writePacket(packet);
            } else if (packet.isReset()) {
                for (int fd2 = fdMin; fd2 <= fdMax; fd2++) {
                    connection *con = connections[fd2 - fdMin];
                    if (con->state == closed)break;//warn: could cause issues
                    if (packet.isFrom(con->src)) {
                        closeConnection(con);
                        break;
                    }
                }
            } else if (packet.isFin()) {
                for (int fd2 = fdMin; fd2 <= fdMax; fd2++) {
                    connection *con = connections[fd2 - fdMin];
                    if (con->state == closed)continue;
                    if (packet.isFrom(con->src)) { //info: assuming it is to the same server
                        switch (con->state) {
                            case established: {
                                unsigned int rInd = con->rcvNxt - con->rcvSeq;
                                unsigned int sInd = con->rcvUsr - con->rcvSeq;

                                unsigned int total = packet.copyDataTo(con->rBuf + rInd, rInd);
                                unsigned int sent = send(con->fd, con->rBuf + sInd,
                                                         con->rcvNxt - con->rcvUsr + total, 0);

                                con->rcvNxt++;
                                create_ack(packet, len, server, client, con->sndUna, con->rcvNxt);
                                unsigned int size = min(con->sndNxt - con->sndUna, (int) con->mss);
                                memcpy(packet + len, con->sBuf, size);
                                con->state = closeWait;
                                writePacket(tunFd, packet, len + size);
                                break;
                            }
                            case closeWait: {
                                int len = PACKET_SIZE;
                                con->rcvNxt++;
                                create_fin_ack(packet, len, server, client, con->sndUna, con->rcvNxt);
                                unsigned int size = min(con->sndNxt - con->sndUna, (int) con->mss);
                                memcpy(packet + len, con->sBuf, size);
                                con->state = closeWait;
                                writePacket(tunFd, packet, len + size);
                            }
                            default: {
                            }
                        }
                    }
                }
            }
        }

        forEnd:;
    }


}

//loops: send to client, delayed ack, flush to server before close, trim connections
//simultaneous ...