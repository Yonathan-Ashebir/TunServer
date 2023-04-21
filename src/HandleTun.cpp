//
// Created by yoni_ash on 4/15/23.
//

#include "HandleTun.h"

using namespace std;

#define PACKET_SIZE 300
#define RECEIVE_BUFFER_SIZE  200//should be less than PACKET_SIZE - 60
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
    sockaddr_in src{};
    sockaddr_in dest{};
    u_int16_t mss{};
    u_char *sBuf{}; //buffer to send data to client
    int sndLen{}; //size of buffer
    int sndWnd{}; // max size of data for next segment
    int sndUna{}; //the sequence number of next octet/data to send to the client
    int sndNxt{}; //the sequence number of new data to send

    u_char rBuf[RECEIVE_BUFFER_SIZE]{}; //buffer to collect data from client
    int rcvNxt{}; //the sequence number of next expected data
    int rcvUsr{}; //the sequence number of next data to be consumed by the user or in our case send to the application server
};


int generateSequenceNo() {
    return rand() % 2 ^ 31;
}

iphdr &getIpHeader(u_char *packet) {
    return *(iphdr *) packet;
}

tcphdr &getTcpHeader(u_char *packet) {
    return *(tcphdr *) (packet + sizeof(iphdr));
}

uint32_t parseWindowSize(u_char *packet, int len) {
    tcphdr tcp = getTcpHeader(packet);
    int result = ntohs(tcp.window);
    for (u_char *ptr = packet + MIN_HEADER_SIZE; ptr < packet + IP_HEADER_SIZE + tcp.doff;) {
        switch (*ptr) {
            case 0:
                return result;
            case 1:
                ptr++;
                continue;
            case 8: {
                u_char len = *(ptr + 1);
                if (len != 3) {
                    printf("Encountered invalid window scale option");
                    return result;
                }
                short shift = 0;
                memcpy(&shift, ptr + 2, 1);
                shift = ntohs(shift);
                result <<= shift;
                return result;
            }
            default:
                ptr += *(ptr + 1);
        }
    }
    printf("Invalid options end encountered");
    return result;
}

uint16_t parseMSS(u_char *packet, int len) {
    for (u_char *ptr = packet + MIN_HEADER_SIZE; ptr < packet + len;) {
        switch (*ptr) {
            case 0:
                return SEND_BUFFER_SIZE;
            case 1:
                ptr++;
                continue;
            case 2: {
                u_char optionLen = *(ptr + 1);
                u_int32_t val = 0;
                memcpy(&val, ptr + 2, optionLen - 2);
                return ntohs(val);
            }
            default:
                ptr += *(ptr + 1);
        }
    }

}


void checkSum(u_char *packet, int len) {

}

inline bool checkFlag(const u_char *packet, int offset) {
    const int flagsOffset = IP_HEADER_SIZE + 13;
    return packet[flagsOffset] & 1 << offset;
}

inline bool checkFinFlag(u_char *packet) {
    return checkFlag(packet, 0);
}

inline bool checkSynFlag(u_char *packet) {
    return checkFlag(packet, 1);
}

inline bool checkRstFlag(u_char *packet) {
    return checkFlag(packet, 2);
}

inline bool checkPshFlag(u_char *packet) {
    return checkFlag(packet, 3);
}

inline bool checkAckFlag(u_char *packet) {
    return checkFlag(packet, 4);
}

inline void setFlag(u_char *packet, int offset, bool val) {
    const int flagsOffset = IP_HEADER_SIZE + 13;
    if (val)
        packet[flagsOffset] |= 1 << offset;
    else packet[flagsOffset] &= ~(1 << offset);
}

inline void setFinFlag(u_char *packet, bool val) {
    setFlag(packet, 0, val);
}

inline void setSynFlag(u_char *packet, bool val) {
    setFlag(packet, 1, val);
}

inline void setRstFlag(u_char *packet, bool val) {
    setFlag(packet, 2, val);
}

inline void setPshFlag(u_char *packet, bool val) {
    setFlag(packet, 3, val);
}

inline void setAckFlag(u_char *packet, bool val) {
    setFlag(packet, 4, val);
}

inline bool isFrom(u_char *packet, sockaddr_in &addr) {
    iphdr ip = getIpHeader(packet);
    tcphdr tcp = getTcpHeader(packet);
    return ip.saddr == addr.sin_addr.s_addr && tcp.source == addr.sin_port;
}

inline bool isTo(u_char *packet, sockaddr_in &addr) {
    iphdr ip = getIpHeader(packet);
    tcphdr tcp = getTcpHeader(packet);
    return ip.daddr == addr.sin_addr.s_addr && tcp.dest == addr.sin_port;
}

int createTcpSocket() {
    int result = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);//warn: might not work on windows
    return result;
}

inline void
create_syn(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, uint16_t mss); //info: Not implemented yet

inline void create_ack(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, uint32_t seqNo, uint32_t ackNo) {

}

inline void
create_syn_ack(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, uint32_t seqNo, uint32_t ackNo,
               int mss) {
    create_ack(packet, len, src, dest, seqNo, ackNo);
    tcphdr tcp = getTcpHeader(packet);

    tcp.window = RECEIVE_BUFFER_SIZE;

    //set mss
    packet[MIN_HEADER_SIZE] = 2;
    packet[MIN_HEADER_SIZE + 1] = 2;

    uint16_t ss = htons(min((uint16_t) (mss % 2 ^ 16), (uint16_t) PACKET_SIZE));
    memcpy(packet + MIN_HEADER_SIZE + 2, &ss, sizeof(short));

    len = MIN_HEADER_SIZE + 4;

}

inline void
create_data(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, int seqNo, int ackNo, u_char *data,
            int dataLen) {
}

inline void create_reset(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, int seqNo, int ackNo) {

}

inline void create_fin(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, int seqNo, int ackNo) {}

inline void create_fin_ack(u_char *packet, int &len, sockaddr_in &src, sockaddr_in &dest, int seqNo, int ackNo) {}


//handleTun
connection *connections[MAX_CONNECTIONS];
u_char packet[PACKET_SIZE];
int fdMin;
int fdMax;

connection createConnection(int fd, sockaddr_in src, sockaddr_in dest, uint32_t seqNo, uint32_t ackNo, uint32_t window,
                            u_int16_t mss) {
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

    res->rcvUsr = ackNo;
    res->rcvNxt = ackNo;

    res->sndUna = seqNo;
    res->sndNxt = seqNo + 1;
    res->sndWnd = window;

    u_int32_t sendLen = window * 2;
    if (sendLen > res->sndLen) {
        res->sndLen = sendLen;
        delete res->sBuf;
        res->sBuf = (u_char *) malloc(sendLen);
    }
    return *connection;
}

inline void closeConnection(connection *con) {
    CLOSE(con->fd);
    con->state = closed;
}

inline int readPacket(int tunFd, u_char *packet, int len) {
    checkSum(packet, MIN_HEADER_SIZE + 4);
    //todo
}

inline int writePacket(int tunFd, u_char *packet, int len) {
    checkSum(packet, MIN_HEADER_SIZE + 4);
    //todo
}

//inline int readData(int fd, u_char* buff, uint32_t len) {
//    int
//}
//
//inline int writeData() {}

void handleTun(int tunFd) {
    //packet buffer for reusing
    fdMax = fdMin = tunFd;
    fd_set master;
    fd_set rcvSet;

    timeval timeout = {0, 10000};

    FD_ZERO(&master);
    FD_ZERO(&rcvSet); //warn: not sure if this is necessary

    FD_SET(tunFd, &master);
    while (true) {
        rcvSet = master;
        int count;
        if ((count = select(fdMax + 1, &rcvSet, nullptr, nullptr, &timeout)) == -1) {
            perror("Select failed at handleTun");
            exit(4);
        }

        //handle each fd
        for (int fd = fdMin; fd <= fdMax && count > 0; fd++) {
            if (FD_ISSET(fd, &rcvSet)) {
                if (fd == tunFd) {
                    readPacket(fd, packet, PACKET_SIZE);
                    iphdr ip = getIpHeader(packet);
                    tcphdr tcp = getTcpHeader(packet);
                    sockaddr_in src = {AF_INET, tcp.source, ip.saddr};
                    sockaddr_in dest = {AF_INET, tcp.dest, ip.daddr};


                    if (checkSynFlag(packet)) {
                        bool success = false;
                        if (checkAckFlag(packet)) {
                            printf("Accepting syn_ack is not supported\n");//todo: this is not right. It might just need ack, with current snd.nxt, rcv.nxt
                        } else {
                            int sock = createTcpSocket();
                            if (sock < 0) {
                                printf("Could not create socket");
                            }
                            if (sock >= 0) {
                                if (connect(sock, (sockaddr *) ip.daddr, sizeof(sockaddr_in)) == 0) {
                                    int len = PACKET_SIZE;
                                    uint32_t syn = generateSequenceNo();
                                    uint32_t windowSize = parseWindowSize(packet, len);
                                    uint16_t mss = parseMSS(packet, len);

                                    createConnection(sock, src, dest, syn, tcp.syn + 1, windowSize, mss);
                                    create_syn_ack(packet, len, src,
                                                   dest,
                                                   syn,
                                                   tcp.syn + 1,
                                                   windowSize);
                                    writePacket(tunFd, packet, len);
                                    success = true; //todo: might not be nessecary
                                    continue;
                                }
                            }
                        }
                        if (success) {
                        } else {
                            int len = PACKET_SIZE;
                            create_reset(packet, len, src,
                                         dest, generateSequenceNo(),
                                         tcp.syn + 2);
                            writePacket(tunFd, packet, len);
                            continue;
                        }
                    } else if (checkRstFlag(packet)) {
                        for (int fd2 = fdMin; fd2 <= fdMax; fd2++) {
                            connection *con = connections[fd2 - fdMin];
                            if (isFrom(packet, con->src)) {
                                closeConnection(con);
                                continue;
                            }
                        }
                    } else if (checkFinFlag(packet)) {
                        for (int fd2 = fdMin; fd2 <= fdMax; fd2++) {
                            connection *con = connections[fd2 - fdMin];
                            if (isFrom(packet, con->src)) { //info: assuming it is to the same dest
                                if (con->state == established) {
                                    int len = PACKET_SIZE;
                                    con->rcvNxt++;
                                    create_ack(packet, len, dest, src, con->sndUna, con->rcvNxt);
                                    uint32_t size = min(con->sndNxt - con->sndUna, (int) con->mss);
                                    memcpy(packet + len, con->sBuf, size);
                                    con->state = closeWait;
                                    writePacket(tunFd, packet, len + size);
                                } else if (con->state == closeWait) {
                                    int len = PACKET_SIZE;
                                    con->rcvNxt++;
                                    create_fin_ack(packet, len, dest, src, con->sndUna, con->rcvNxt);
                                    uint32_t size = min(con->sndNxt - con->sndUna, (int) con->mss);
                                    memcpy(packet + len, con->sBuf, size);
                                    con->state = closeWait;
                                    writePacket(tunFd, packet, len + size);
                                }
                                continue;
                            }
                        }
                    }
                } else {

                }
                count--;
            }
        }

    }
}

//loops: send to client, delayed ack, flush to server before close, trim connections
//simultaneous ...