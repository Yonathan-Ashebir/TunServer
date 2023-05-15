//
// Created by yoni_ash on 4/15/23.
//

#ifndef TUNSERVER_INCLUDE_H
#define TUNSERVER_INCLUDE_H

#pragma once

#include <strings.h>

#include <cstdio>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <cmath>
#include <fcntl.h>
#include <random>
#include <string>
#include "error/Error.h"
#include <mutex>
#include <thread>
#include <sys/types.h>

//#define STRICT_MODE
//#define LOGGING

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#define bzero(b, len) (memset((b), '\0', (len)), (void) 0)

#include <winsock2.h>
#include <ws2tcpip.h>

#define CLOSE closesocket
#define random rand
#define socket_t SOCKET
#define SHUT_WR SD_SEND
#define SHUT_RD SD_RECEIVE
#define BUFFER_BYTE char

struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char ihl: 4;
    unsigned char version: 4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error        "Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
};

struct tcphdr {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    uint16_t res1: 4;
    uint16_t doff: 4;
    uint16_t fin: 1;
    uint16_t syn: 1;
    uint16_t rst: 1;
    uint16_t psh: 1;
    uint16_t ack: 1;
    uint16_t urg: 1;
    uint16_t res2: 2;
#  elif __BYTE_ORDER == __BIG_ENDIAN
    uint16_t doff:4;
    uint16_t res1:4;
    uint16_t res2:2;
    uint16_t urg:1;
    uint16_t ack:1;
    uint16_t psh:1;
    uint16_t rst:1;
    uint16_t syn:1;
    uint16_t fin:1;
#  else
#   error "Adjust your <bits/endian.h> defines"
#  endif
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};


#else

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CLOSE close
#define socket_t int
#define BUFFER_BYTE unsigned char

#endif


using namespace std;

inline void printError(const char *tag, int errNo) {
    ::printf("%s with error no: %d, error name: %s, desc: %s\n", tag, errNo,
             getErrorName(errno), ::strerror(errNo));
}

inline void printError(const string &tag, int errNo) {
    printError(tag.c_str(), errNo);
}

inline void printError(const char *tag) {
    printError(tag, errno);
}

inline void printError(const string &tag) {
    printError(tag.c_str(), errno);
}

inline void exitWithError(const char *tag) {
    printError(tag);
    exit(errno);
}

inline void exitWithError(const string &tag) {
    exitWithError(tag.c_str());
}
template<typename T>
inline void shiftElements(T *from, unsigned int len, int by) {
    len *= sizeof (T);
    by *= sizeof (T);
    unsigned char temp[len];
    memcpy(temp, from, len);
    memcpy(from + by, temp, len);
}

inline void initPlatform() {

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        fprintf(stderr, "Versiion 2.2 of Winsock is not available.\n");
        WSACleanup();
        exit(2);
    }
#endif
}


#endif //TUNSERVER_INCLUDE_H