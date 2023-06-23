//
// Created by yoni_ash on 6/22/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_SOCKETIMP_H
#define TUNSERVER_SOCKETIMP_H

#include "Socket.h"

/*Socket Implementations*/
Socket::Socket(int domain, int type, int proto) : Socket(socket(domain, type, proto), domain, type, proto) {
}

Socket::~Socket() {
    if (data != nullptr) {
        if (--data->count == 0) {
            CLOSE(data->socket);
            delete data;
        }
    }
}

Socket::Socket(Socket &&old) noexcept {
    data = old.data;
    old.data = nullptr;
}

Socket &Socket::operator=(const Socket &old) {
    if (&old != this) {
        if (data != nullptr && --data->count == 0) {
            CLOSE(data->socket);
            delete data;
        }
        data = old.data;
        data->count++;
    }

    return *this;
}

Socket::Socket(const Socket &old) {
    data = old.data;
    data->count++;
}

Socket &Socket::operator=(Socket &&old) noexcept {
    if (&old != this) {
        if (data != nullptr && --data->count == 0) {
            CLOSE(data->socket);
            delete data;
        }
        data = old.data;
        old.data = nullptr;
    }
    return *this;
}

void Socket::setOption(int level, int option, void *val, socklen_t len) {
    if (setsockopt(data->socket, level, option, (char *) val, len) == -1) {
        throw SocketException("Could not set an option on socket");
    }
}

void Socket::getOption(int level, int option, void *val, socklen_t &len) {
    if (getsockopt(data->socket, level, option, (char *) val, &len) == -1) {
        throw SocketException("Could not get an option from socket");
    }
}

void Socket::setSocketOption(int option, void *val, socklen_t len) {
    setOption(SOL_SOCKET, option, val, len);
}

void Socket::getSocketOption(int option, void *val, socklen_t &len) {
    getOption(SOL_SOCKET, option, val, len);
}

void Socket::setIn(fd_set &set) {
    FD_SET(data->socket, &set);
}

void Socket::unsetFrom(fd_set &set) {
    FD_CLR(data->socket, &set);
}

bool Socket::isSetIn(fd_set &set) {
    return FD_ISSET(data->socket, &set);
}

socket_t Socket::getFD() {
    return data->socket;
}

int Socket::getLastError() {
    int err;
    getSocketOption(SO_ERROR, err);
    return err;
}

void Socket::setLinger(linger &ln) {
    setSocketOption(SO_LINGER, ln);
}

template<class Val>
void Socket::getSocketOption(int option, Val &val) {
    getOption(SOL_SOCKET, option, val);
}

template<class Val>
void Socket::setSocketOption(int option, Val &val) {
    setOption(SOL_SOCKET, option, val);
}

template<class Val>
void Socket::getOption(int level, int option, Val &val) {
    socklen_t len = sizeof val;
    getOption(level, option, &val, len);
    if (len > sizeof val)
        throw length_error("Socket option value trimmed");
}

template<class Val>
void Socket::setOption(int level, int option, Val &val) {
    setOption(level, option, &val, sizeof val);
}

/*InetSocket implementations*/
InetSocket::InetSocket(int type, bool ipv6) : Socket(ipv6 ? AF_INET6 : AF_INET, type, 0) {
}

void InetSocket::setBlocking(bool isBlocking) {
#ifdef  _WIN32
    if (ioctlsocket(data->socket, FIONBIO, isBlocking ? 0 :(char *) &INT_ONE) == -1)
            throw SocketException("Could not set non blocking flag on socket");
#else
    if (isBlocking) {
        auto flags = fcntl(data->socket, F_GETFL);
        if (flags == -1)throw GeneralException("Could not read socket mSock flags");
        if (fcntl(data->socket, F_SETFL, flags & ~O_NONBLOCK) == -1)
            throw GeneralException("Could not set socket to block");
    } else if (fcntl(data->socket, F_SETFL, O_NONBLOCK) == -1)
        throw GeneralException("Could not set non blocking flag on socket");
#endif
}

void InetSocket::setReuseAddress(bool reuse) {
    setSocketOption(SO_REUSEADDR, reuse ? (char *) &INT_ONE : (char *) &ZERO,
                    sizeof(int));
}

int InetSocket::tryBind(void *addr, socklen_t len) {
    return ::bind(data->socket, reinterpret_cast<const sockaddr *>(addr), len);
}

void InetSocket::bind(void *addr, socklen_t len) {
    if (tryBind(addr, len) == -1)
        throw SocketException("Could not bind");
}

void InetSocket::bind(unsigned short port) {
    sockaddr_storage addr{};
    initialize(addr, static_cast<sa_family_t>(data->domain), nullptr, port);
    bind(addr);
}

void InetSocket::bind(const char *ip, unsigned short port) {
    sockaddr_storage addr{static_cast<sa_family_t>(data->domain)};
    initialize(addr, static_cast<sa_family_t>(data->domain), ip, port);
    bind(addr);
}

void InetSocket::bind(const string &ip, unsigned short port) {
    bind(ip.c_str(), port);
}

int InetSocket::tryConnect(void *addr, socklen_t len) {
    return ::connect(data->socket, reinterpret_cast<const sockaddr *>(addr), len);
}

void InetSocket::connect(void *addr, socklen_t len) {
    if (tryConnect(addr, len) == -1)
        throw SocketException("Could not connect");
}

void InetSocket::connect(const char *ip, unsigned short port) {
    sockaddr_storage addr{static_cast<sa_family_t>(data->domain)};
    initialize(addr, static_cast<sa_family_t>(data->domain), ip, port);
    connect(addr);
}

void InetSocket::connect(const string &ip, unsigned short port) {
    connect(ip.c_str(), port);
}

void InetSocket::connectToHost(const char *hostname, unsigned short port) {
    auto addrInfo = resolveHost(hostname, "80", data->domain, data->type);
    if (addrInfo->ai_family == AF_INET)reinterpret_cast<sockaddr_in *>(addrInfo->ai_addr)->sin_port = htons(port);
    else reinterpret_cast<sockaddr_in6 *>(addrInfo->ai_addr)->sin6_port = htons(port);
    connect(addrInfo->ai_addr, addrInfo->ai_addrlen);
}

int InetSocket::trySend(const void *buf, int len, int options) {
    return ::send(data->socket, (char *) buf, len, options); // NOLINT(cppcoreguidelines-narrowing-conversions)
}

int InetSocket::send(const void *buf, int len, int options) {
    int sent;
    if ((sent = trySend(buf, len, options)) == -1)throw SocketException("Could not send");
    return sent;
}

int InetSocket::tryReceive(void *buf, int len, int options) {
    return ::recv(data->socket, (char *) buf, len, options); // NOLINT(cppcoreguidelines-narrowing-conversions)
}

int InetSocket::receive(void *buf, int len, int options) {
    int sent;
    if ((sent = tryReceive(buf, len, options)) == -1)throw SocketException("Could not send");
    return sent;
}

void Socket::close() {
    if (data->socket != -1 && CLOSE(data->socket) == -1)
        throw SocketException("Could not close the socket");
    data->socket = -1;
}

bool InetSocket::getReuseAddress() {
    int res{};
    getSocketOption(SO_REUSEADDR, res);
    return res;
}

void InetSocket::getBindAddress(void *addr, socklen_t &len) {
    if (getsockname(data->socket, reinterpret_cast<sockaddr *>(addr), &len) == -1)
        throw SocketException("Could query the socket's bind address");
}

unique_ptr<sockaddr_storage> InetSocket::getBindAddress() {
    unique_ptr<sockaddr_storage> result{new sockaddr_storage{}};
    getBindAddress(*result);
    return result;
}

void Socket::regenerate() {
    close();
    if ((data->socket = ::socket(data->domain, data->type, data->protocol)) == -1)
        throw SocketException("Could not create a new socket at regenerate()");
}

void Socket::shutdownWrite() {
    shutdown(data->socket, SHUT_WR);
}

void Socket::shutdownRead() {
    shutdown(data->socket, SHUT_RD);
}

Socket::Socket(int sock, int domain, int type, int proto) {
    if (sock == -1)throw SocketException("Could not create a socket");
    data->socket = sock;
    data->domain = domain;
    data->type = type;
    data->protocol = proto;
}

template<class Addr>
void InetSocket::getBindAddress(Addr &addr) {
    socklen_t len = sizeof addr;
    getBindAddress(&addr, len);
    if (len > sizeof addr)throw length_error("Bind address trimmed");
}

template<class Addr>
void InetSocket::connect(Addr &addr) {
    if (tryConnect(addr) == -1)
        throw SocketException("Could not connect");
}

template<class Addr>
int InetSocket::tryConnect(Addr &addr) {
    return ::connect(data->socket, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
}

template<class Addr>
void InetSocket::bind(Addr &addr) {
    if (tryBind(addr) == -1)
        throw SocketException("Could not bind");
}

template<class Addr>
int InetSocket::tryBind(Addr &addr) {
    return ::bind(data->socket, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
}

/*TCPSocket Implementations*/
void TCPSocket::setTCPOption(int option, void *val, socklen_t len) {
    setOption(IPPROTO_TCP, option, val, len);
}

void TCPSocket::getTCPOption(int option, void *val, socklen_t &len) {
    getOption(IPPROTO_TCP, option, val, len);
}

void TCPSocket::enableNagle(bool enable) {
    setTCPOption(TCP_NODELAY, enable ? (char *) &ZERO : (char *) &INT_ONE, sizeof(int));
}

TCPSocket::TCPSocket(bool ipv6) : InetSocket(SOCK_STREAM, ipv6) {}

int TCPSocket::tryListen(int count) {
    return ::listen(data->socket, count);
}

void TCPSocket::listen(int count) {
    if (tryListen(count) == -1)throw SocketException("Could not listen");
}


TCPSocket TCPSocket::accept(void *addr, socklen_t &len) {
    socket_t res;
    if ((res = ::accept(data->socket, reinterpret_cast<sockaddr *>(addr), &len)) == -1)
        throw SocketException("Could not accept");
    Socket sock(res, data->domain, data->type, data->protocol);
    return *reinterpret_cast<TCPSocket *>(&sock);
}

template<class Addr>
TCPSocket TCPSocket::accept(Addr addr) {
    socklen_t len = sizeof addr;
    auto res = accept(&addr, len);
    if (len > sizeof addr)throw length_error("Peer ip trimmed");
    return res;
}

template<class Val>
void TCPSocket::getTCPOption(int option, Val &val) {
    getOption(IPPROTO_TCP, option, val);
}

template<class Val>
void TCPSocket::setTCPOption(int option, Val &val) {
    setOption(IPPROTO_TCP, option, val);
}

/*UDPSocket Implementations*/
void UDPSocket::getUDPOption(int option, void *val, socklen_t &len) {
    getOption(IPPROTO_UDP, option, val, len);
}

void UDPSocket::setUDPOption(int option, void *val, int len) {
    setOption(IPPROTO_UDP, option, val, len);
}

UDPSocket::UDPSocket(bool ipv6) : InetSocket(SOCK_DGRAM, ipv6) {
}

template<class Val>
void UDPSocket::getUDPOption(int option, Val &val) {
    getOption(IPPROTO_UDP, option, val);
}

template<class Val>
void UDPSocket::setUDPOption(int option, Val &val) {
    setOption(IPPROTO_UDP, option, val);
}

//todo: moved socket protection
#endif //TUNSERVER_SOCKETIMP_H

#pragma clang diagnostic pop