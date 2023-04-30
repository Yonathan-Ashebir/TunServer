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

#define STRICT_MODE

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#include <winsock2.h>
#define CLOSE closesocket

#else

#include <netinet/in.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CLOSE close

#endif

using namespace std;

using namespace std;

inline void printError(const char *tag, int errNo) {
    ::printf("%s with error no: %d, error name: %s, desc: %s\n", tag, errNo,
             getErrorName(errno), ::strerror(errNo));
}

inline void printError(const char *tag) {
    printError(tag, errno);
}

inline void exitWithError(const char *tag) {
    printError(tag);
    exit(errno);
}

inline void shiftElements(unsigned char *from, unsigned int len, int by) {
    unsigned char temp[len];
    memcpy(temp, from, len);
    memcpy(from + by, temp, len);
}

inline void initPlatform() {

#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    WSADATA wsaData;
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

//#include "Include.cpp"

#endif //TUNSERVER_INCLUDE_H
