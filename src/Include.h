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
#include <mutex>
#include <thread>
#include <csignal>
#include <sys/types.h>
#include <stun++/message.h>
#include "error/Error.h"

//#define STRICT_MODE
#define LOGGING
const bool _SOCKET_VAL_BOOL = true;
const char SOCKET_VAL_BOOL = *(char *)&_SOCKET_VAL_BOOL;
const int _SOCKET_VAL_INT = 1;
const char SOCKET_VAL_INT = *(char *)&_SOCKET_VAL_INT;
const unsigned long _SOCKET_VAL_ULONG = 1ul;
const char SOCKET_VAL_ULONG = *(char *)&_SOCKET_VAL_ULONG;

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
#define SOCKET_OPTION_POINTER_CAST (char *)

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
#include <functional>

#define CLOSE close
#define socket_t int
#define BUFFER_BYTE unsigned char
#define SOCKET_OPTION_POINTER_CAST (int *)

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

inline const char *formatError(const char *msg, int err = errno) {
    char *buf = new char[256];
    if (snprintf(buf, 256, ERROR_FORMAT, msg, err,
                 getErrorName(err), ::strerror(err)) == -1)
        exitWithError("Failed to format the error message");
    return buf;
}

inline string formatError(const string &msg, int err = errno) {
    return string{formatError(msg.c_str(), err)};
}

class FormattedException : public exception {
public:
    explicit FormattedException(const char *msg, int err = errno) : msg(formatError(msg, err)) {

    }

    explicit FormattedException(string &msg, int err = errno) : msg(formatError(msg, err)) {

    }


    [[nodiscard]] const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
        return (new string(msg))->c_str();
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
        fprintf(stderr, "Versiion 2.2 of Winsock is not available.\n");
        WSACleanup();
        exit(2);
    }
#endif
}

inline socket_t createTcpSocket(bool reuseAddress = false, bool nonBlocking = false, bool disableNagle = false) {
    socket_t result;
#ifdef _WIN32
    result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (result == INVALID_SOCKET)exitWithError("Could not create socket");
#else
    result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (result == -1)exitWithError("Could not create the socket");
#endif

    if (reuseAddress)
        if (setsockopt(result, SOL_SOCKET, SO_REUSEADDR, &SOCKET_VAL_INT, sizeof(SOCKET_VAL_INT)) == -1)
            exitWithError("Could not disable Nagle algorithm");
    if (nonBlocking) {
#ifdef  _WIN32
        auto mode = 1UL;
        if (ioctlsocket(result, FIONBIO, &mode) == -1)exitWithError("Could not set non blocking flag on socket");
#else
        if (fcntl(result, F_SETFL, O_NONBLOCK) == -1)exitWithError("Could not set non blocking flag on socket");
#endif
    }

    if (disableNagle)
        if (setsockopt(result, IPPROTO_TCP, TCP_NODELAY, &SOCKET_VAL_INT, sizeof(SOCKET_VAL_INT)) == -1)
            exitWithError("Could not disable Nagle algorithm");


    return result;
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
        if (setsockopt(result, SOL_SOCKET, SO_REUSEADDR, &SOCKET_VAL_INT, sizeof(SOCKET_VAL_INT)) == -1)
            exitWithError("Could not disable Nagle algorithm");

    if (nonBlocking) {
#ifdef  _WIN32
        auto mode = 1UL;
        if (ioctlsocket(result, FIONBIO, &mode) == -1)exitWithError("Could not set non blocking flag on socket");
#else
        if (fcntl(result, F_SETFL, O_NONBLOCK) == -1)exitWithError("Could not set non blocking flag on socket");
#endif
    }

    if (disableNagle)
        if (setsockopt(result, IPPROTO_TCP, TCP_NODELAY, &SOCKET_VAL_INT, sizeof(SOCKET_VAL_INT)) == -1)
            exitWithError("Could not disable Nagle algorithm");


    return result;
}


inline sockaddr_storage *getTunServerAddress(bool forceRefresh = false) {
    auto result = new sockaddr_storage{AF_INET};
    auto result_in = reinterpret_cast<sockaddr_in *>(result);

    addrinfo hints{0, AF_INET, SOCK_STREAM}, *addresses;
    int code;
    if ((code = getaddrinfo("stun.sipnet.net", "http", &hints, &addresses)) != 0) {
        exitWithError("Could not resolve host with error " + string(gai_strerror(code)));
    }
    *result_in = *reinterpret_cast<sockaddr_in *>(addresses->ai_addr);
    result_in->sin_port = htons(3478);
    freeaddrinfo(addresses);

    return result;
}

inline sockaddr_storage *getTcpMappedAddress(socket_t sock, bool isConnected = false) {
    auto stunAddr = getTunServerAddress();
    if (!isConnected && connect(sock, reinterpret_cast<const sockaddr *>(stunAddr), sizeof(sockaddr_in)) ==
                        -1)
        exitWithError("Could not connect to stun server");
    stun::message request(stun::message::binding_request,
    new unsigned char[12]);
    if (send(sock, (char *) request.data(), request.size(), 0) == -1)exitWithError("Could not send request");

    stun::message response{};

    size_t bytes = recv(sock, (char *) response.data(), response.capacity(), 0);
    if (bytes == -1) throw FormattedException("Could not read stun repose header");
    if (bytes < stun::message::header_size)exitWithError("Response trimmed");
    if (response.size() > response.capacity()) {
        size_t read_bytes = response.size();
        response.resize(response.size());
        recv(sock, (char *) response.data() + bytes, read_bytes - bytes, 0);
    }

    using namespace stun::attribute;
    auto address = new sockaddr_storage{AF_INET};

    for (auto &attr: response) {
        // First, check the attribute type
        switch (attr.type()) {
            case type::mapped_address:
                attr.to<type::mapped_address>().to_sockaddr((sockaddr *) address);
                break;
            case type::xor_mapped_address:
                attr.to<type::xor_mapped_address>().to_sockaddr((sockaddr *) address);
                return address;
        }
    }
    return address;
}

inline sockaddr_storage *getTcpMappedAddress(sockaddr_storage &bindAddr) {
    auto sock = createTcpSocket(true);
    if (bind(sock, reinterpret_cast<const sockaddr *>(&bindAddr), sizeof(bindAddr)) == -1)
        throw system_error(error_code(errno, system_category()));
    auto result = getTcpMappedAddress(sock);
    CLOSE(sock);
    return result;
}

inline bool isConnectionInProgress() {
#ifdef _WIN32
    auto lastError = WSAGetLastError();
#endif
    return
#ifdef _WIN32
            (lastError == 0 || lastError == WSAEWOULDBLOCK || lastError == WSAEINPROGRESS ||
             lastError == WSAEALREADY) &&
            #endif
            (errno == 0 || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY);
}

inline bool isWouldBlock() {
#ifdef _WIN32
    auto lastError = WSAGetLastError();
#endif
    return
#ifdef _WIN32
            (lastError == WSAEWOULDBLOCK) &&
            #endif
            (errno == EWOULDBLOCK || errno == EAGAIN);
}

#endif //TUNSERVER_INCLUDE_H