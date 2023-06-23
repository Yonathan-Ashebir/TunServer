//
// Created by DELL on 6/21/2023.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_HELPERS_INCLUDE_H
#define TUNSERVER_HELPERS_INCLUDE_H

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
#include <memory>
#include <random>
#include <ranges>
#include <string>
#include <mutex>
#include <thread>
#include <functional>
#include <memory>
#include <csignal>
#include <sys/types.h>
#include "stun++/message.h"
#include "Error.h"

//#define STRICT_MODE
#define LOGGING
#define random rand
const int INT_ONE = 1;
const int ULONG_ONE = 1ul;
const long long ZERO = 0;
#define GeneralException(msg) std::system_error(errno,system_category(),msg)

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#define bzero(b, len) (memset((b), '\0', (len)), (void) 0)

#include <winsock2.h>
#include <ws2tcpip.h>

#define CLOSE closesocket
#define socket_t SOCKET
#define SHUT_WR SD_SEND
#define SHUT_RD SD_RECEIVE

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

#define CLOSE ::close
#define socket_t int

#endif


using namespace std;

#define ERROR_FORMAT "%s with error num: %d | name: %s | description: %s\n"


inline void printError(const char *msg, int err = errno) {
    //todo: finalize
#ifdef _WIN32
    if (err == 0)err = WSAGetLastError();
#endif
    ::printf(ERROR_FORMAT, msg, err,
             getErrorName(err), std::system_category().message(err).c_str());
}

inline void printError(const string &msg, int err = errno) {
    printError(msg.c_str(), err);
}

inline void exitWithError(const char *msg, int err = errno) {
    printError(msg, err);
    exit(err);
}

inline void exitWithError(const string &msg, int err = errno) {
    exitWithError(msg.c_str(), err);
}

inline unique_ptr<const char> formatError(const char *msg, int err = errno) {
    auto buf = unique_ptr<char>{new char[256]};
    if (snprintf(buf.get(), 256, ERROR_FORMAT, msg, err,
                 getErrorName(err), ::strerror(err)) == -1)
        exitWithError("Failed to format the error message");
    return buf;
}

inline string formatError(const string &msg, int err = errno) {
    return string{formatError(msg.c_str(), err).get()};
}

class BadException : public bad_exception {
public:
    explicit BadException(const char *msg, int err = errno) : msg(formatError(msg, err).get()) {

    }

    explicit BadException(string &msg, int err = errno) : msg(formatError(msg, err)) {

    }


    const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
        return msg.c_str();
    }

private:
    const string msg;
};

template<typename T>
inline void shiftElements(T *from, unsigned int len, int by) {
    len *= sizeof(T);
    by *= sizeof(T);
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
        fprintf(stderr, "Version 2.2 of Winsock is not available.\n");
        WSACleanup();
        exit(2);
    }
#endif
}


inline socket_t createUdpSocket(bool reuseAddress = false, bool nonBlocking = false, bool disableNagle = false) {
    socket_t result;
#ifdef _WIN32
    result = socket(AF_INET, SOCK_DGRAM, 0);
    if (result == INVALID_SOCKET)exitWithError("Could not create socket");
#else
    result = socket(AF_INET, SOCK_DGRAM, 0);
    if (result == -1)exitWithError("Could not create the socket");
#endif


    if (reuseAddress)
        if (setsockopt(result, SOL_SOCKET, SO_REUSEADDR,(char *) &INT_ONE, sizeof(int)) == -1)
            exitWithError("Could not set reuse address");

    if (nonBlocking) {
#ifdef  _WIN32
        auto mode = 1UL;
        if (ioctlsocket(result, FIONBIO, &mode) == -1)exitWithError("Could not set non blocking flag on socket");
#else
        if (fcntl(result, F_SETFL, O_NONBLOCK) == -1)exitWithError("Could not set non blocking flag on socket");
#endif
    }

    if (disableNagle)
        if (setsockopt(result, IPPROTO_TCP, TCP_NODELAY,(char *) &INT_ONE, sizeof(int)) == -1)
            exitWithError("Could not disable Nagle algorithm");


    return result;
}




#endif //TUNSERVER_HELPERS_INCLUDE_H

#pragma clang diagnostic pop