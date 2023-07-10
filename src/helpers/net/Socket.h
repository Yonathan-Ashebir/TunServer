//
// Created by yoni_ash on 6/21/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_SOCKET_H
#define TUNSERVER_SOCKET_H

#include "../Include.h"
#include "SocketError.h"
#include "Utilities.h"

class Socket {
public:
    inline explicit Socket(DeferInit);

    inline Socket(int domain, int type, int proto);

    inline Socket(socket_t fd, int domain, int type, int proto);

    inline socket_t getFD();

    inline Socket(const Socket &old) = default;

    inline Socket(Socket &&old) noexcept = default;

    inline Socket &operator=(const Socket &old) = default;

    inline Socket &operator=(Socket &&old) noexcept = default;

    inline void setOption(int level, int option, void *val, socklen_t len);

    template<class Val>
    inline void setOption(int level, int option, Val &val);

    inline void getOption(int level, int option, void *val, socklen_t &len);

    template<class Val>
    inline void getOption(int level, int option, Val &val);

    template<class Val>
    inline void setSocketOption(int option, Val &val);

    inline void setSocketOption(int option, void *val, socklen_t len);

    template<class Val>
    inline void getSocketOption(int option, Val &val);

    inline void getSocketOption(int option, void *val, socklen_t &len);

    inline void setLinger(linger &ln);

    inline linger getLinger(){
        linger ln{};
        getSocketOption(SO_LINGER,ln);
        return ln;
    }

    inline void setSendTimeout(chrono::milliseconds timeout) {
#ifdef _WIN32
        DWORD val{timeout.count()};
#else
        timeval val{timeout / chrono::seconds(1), (timeout % chrono::seconds(1)).count() * 1000};
#endif
        setSocketOption(SO_SNDTIMEO, val);
    };

    inline chrono::milliseconds getSendTimeout() {
#ifdef _WIN32
        DWORD val{}
        getSocketOption(SO_RCVTIMEO,val);
        return chrono::milliseconds{val};
#else
        timeval val{};
        getSocketOption(SO_SNDTIMEO, val);
        return chrono::milliseconds{val.tv_sec * 1000 + val.tv_usec / 1000};
#endif
    };

    inline void setReceiveTimeout(chrono::milliseconds timeout) {
#ifdef _WIN32
        DWORD val{timeout.count()};
#else
        timeval val{timeout / chrono::seconds(1), (timeout % chrono::seconds(1)).count() * 1000};
#endif
        setSocketOption(SO_RCVTIMEO, val);
    };

    inline chrono::milliseconds getReceiveTimeout() {
#ifdef _WIN32
        DWORD val{}
        getSocketOption(SO_RCVTIMEO,val);
        return chrono::milliseconds{val};
#else
        timeval val{};
        getSocketOption(SO_RCVTIMEO, val);
        return chrono::milliseconds{val.tv_sec * 1000 + val.tv_usec / 1000};
#endif
    };


    inline int getLastError();

    inline void setIn(fd_set &set);

    inline void unsetFrom(fd_set &set);

    inline bool isSetIn(fd_set &set);

    inline void shutdownRead();

    inline void shutdownWrite();

    inline void close();

    inline void regenerate();

    inline bool isValid();

protected:
    struct Data {
        socket_t socket{(socket_t) -1};
        int domain{};
        int type{};
        int protocol{};
        unsigned long count{1};

        Data() = default;

        ~Data() {
            if (socket != -1)CLOSE(socket);
        }

        Data(socket_t fd, int domain, int type, int proto) : socket(fd), domain(domain), type(type), protocol(proto) {}
    };

    shared_ptr<Data> data;

private:
};

class InetSocket : public Socket {
public:
    inline explicit InetSocket(DeferInit _);

    inline explicit InetSocket(int type, bool ipv6 = false);

    inline void setBlocking(bool isBlocking);

    inline void setReuseAddress(bool reuse);

    inline bool getReuseAddress();

    inline int tryBind(void *addr, socklen_t len);

    template<class Addr>
    inline int tryBind(Addr &addr);

    inline void bind(void *addr, socklen_t len);

    template<class Addr>
    inline void bind(Addr &addr);

    inline void bind(unsigned short port);

    inline void bind(const char *ip, unsigned short port);

    inline void bind(const string &ip, unsigned short port);

    inline void getBindAddress(void *addr, socklen_t &len);

    template<class Addr>
    inline void getBindAddress(Addr &addr);

    inline unique_ptr<sockaddr_storage> getBindAddress();;

    inline int tryConnect(void *addr, socklen_t len);

    template<class Addr>
    inline int tryConnect(Addr &addr);

    inline int connect(void *addr, socklen_t len, bool ignoreWouldBlock = false);

    /** The socket is set to non-blocking after this operation*/
    inline void connect(void *addr, socklen_t len, chrono::milliseconds timeout) {
        setBlocking(false);
        if (!connect(addr, len, true))return;
        fd_set snd, err;
        FD_ZERO(&snd);
        FD_ZERO(&err);
        setIn(snd);
        setIn(err);
        timeval timeval{0, timeout.count() * 1000};
        auto count = select(data->socket + 1, nullptr, &snd, &err, &timeval);
        if (count < 0)throw SocketError("select failed on socket");
        if (count == 0)throw SocketError("Connect timed out");
        if (isSetIn(err))throw SocketError("Error occurred during connect");
    }

    inline int connect(unsigned short port, bool ignoreWouldBlock = false);

    inline void connect(unsigned short port, chrono::milliseconds timeout) {
        sockaddr_storage addr{};
        initialize(addr, static_cast<sa_family_t>(data->domain), "127.0.0.1", port);
        connect(addr, timeout);
    }

    template<class Addr>
    inline int connect(Addr &addr, bool ignoreWouldBlock = false);

    template<class Addr>
    inline void connect(Addr &addr, chrono::milliseconds timeout) {
        connect(&addr, sizeof addr, timeout);
    }

    inline int connect(const char *ip, unsigned short port, bool ignoreWouldBlock = false);

    inline void connect(const char *ip, unsigned short port, chrono::milliseconds timeout) {
        sockaddr_storage addr{static_cast<sa_family_t>(data->domain)};
        initialize(addr, static_cast<sa_family_t>(data->domain), ip, port);
        connect(addr, timeout);
    }

    inline int connect(const string &ip, unsigned short port, bool ignoreWouldBlock = false);

    inline void connect(const string &ip, unsigned short port, chrono::milliseconds timeout) {
        connect(ip.c_str(), port, timeout);
    }

    inline int connectToHost(const char *hostname, unsigned short port, bool ignoreWouldBlock = false);

    inline void connectToHost(const char *hostname, unsigned short port, chrono::milliseconds timeout) {
        auto addrInfo = resolveHost(hostname, "80", data->domain, data->type);
        if (addrInfo->ai_family == AF_INET)reinterpret_cast<sockaddr_in *>(addrInfo->ai_addr)->sin_port = htons(port);
        else reinterpret_cast<sockaddr_in6 *>(addrInfo->ai_addr)->sin6_port = htons(port);
        connect(addrInfo->ai_addr, (socklen_t) addrInfo->ai_addrlen, timeout);
    }

    inline int connectToHost(const string &hostname, unsigned short port, bool ignoreWouldBlock = false);

    inline void connectToHost(const string &hostname, unsigned short port, chrono::milliseconds timeout) {
        connect(hostname.c_str(), port, timeout);
    }

    inline int trySend(const void *buf, int len, int options = 0);

    template<class Buffer>
    inline int trySendObject(Buffer &buf, int options = 0);

    inline int send(const void *buf, int len, int options = 0);

    template<class Buffer>
    inline int sendObject(Buffer &buf, int options = 0);

    template<class Buffer>
    inline int sendObject(Buffer &&buf, int options = 0);

    inline int sendIgnoreWouldBlock(const void *buf, int len, int options = 0);

    template<class Buffer>
    inline int sendObjectIgnoreWouldBlock(Buffer &buf, int options = 0);

    template<class Buffer>
    inline int sendObjectIgnoreWouldBlock(Buffer &&buf, int options = 0);

    inline int tryReceive(void *buf, int len, int options = 0);

    template<class Buffer>
    inline int tryReceiveObject(Buffer &buf, int options = 0);

    inline int receive(void *buf, int len, int options = 0);

    template<class Buffer>
    inline int receiveObject(Buffer &buf, int options = 0);

    inline int receiveIgnoreWouldBlock(void *buf, int len, int options = 0);;

    template<class Buffer>
    inline int receiveObjectIgnoreWouldBlock(Buffer &buf, int options = 0);
};


class TCPSocket : public InetSocket {
public:
    inline explicit TCPSocket(DeferInit _);

    inline explicit TCPSocket(bool ipv6 = false);

    template<class Val>
    inline void setTCPOption(int option, Val &val);

    inline void setTCPOption(int option, void *val, socklen_t len);

    template<class Val>
    inline void getTCPOption(int option, Val &val);

    inline void getTCPOption(int option, void *val, socklen_t &len);

    inline void enableNagle(bool enable);

    inline int tryListen(int count = 20);

    inline void listen(int count = 20);

    inline TCPSocket tryAccept(void *addr, socklen_t &len);

    inline TCPSocket accept(void *addr, socklen_t &len);

    template<class Addr>
    inline TCPSocket accept(Addr &addr);

};


class UDPSocket : public InetSocket {
public:
    inline explicit UDPSocket(DeferInit _);

    inline explicit UDPSocket(bool ipv6 = false);

    template<class Val>
    inline void setUDPOption(int option, Val &val);

    inline void setUDPOption(int option, void *val, int len);

    inline void getUDPOption(int option, void *val, socklen_t &len);

    template<class Val>
    inline void getUDPOption(int option, Val &val);

    inline int tryReceiveFrom(void *buf, int len, void *addr, socklen_t &addrLen, int options = 0);

    template<typename Buffer>
    inline int tryReceiveObjectFrom(Buffer &buf, void *addr, socklen_t &addrLen, int options = 0);

    inline int receiveFrom(void *buf, int len, void *addr, socklen_t &addrLen, int options = 0);

    template<typename Addr>
    inline int receiveFrom(void *buf, int len, Addr &addr, int options = 0);

    template<typename Buffer>
    inline int receiveObjectFrom(Buffer &buf, void *addr, socklen_t &addrLen, int options = 0);

    template<typename Buffer, typename Addr>
    inline int receiveObjectFrom(Buffer &buf, Addr &addr, int options = 0);

    inline int receiveFromIgnoreWouldBlock(void *buf, int len, void *addr, socklen_t &addrLen, int options = 0);;

    template<typename Addr>
    inline int receiveFromIgnoreWouldBlock(void *buf, int len, Addr &addr, int options = 0);

    template<typename Buffer>
    inline int receiveObjectFromIgnoreWouldBlock(Buffer &buf, void *addr, socklen_t &addrLen, int options = 0);

    template<typename Buffer, typename Addr>
    inline int receiveObjectFromIgnoreWouldBlock(Buffer &buf, Addr &addr, int options = 0);

    inline int trySendTo(void *buf, int len, void *addr, socklen_t addrLen, int options = 0);

    template<typename Buffer>
    inline int trySendObjectTo(Buffer &buf, void *addr, socklen_t addrLen, int options = 0);

    inline int sendTo(void *buf, int len, void *addr, socklen_t addrLen, int options = 0);

    template<typename Addr>
    inline int sendTo(void *buf, int len, Addr &addr, int options = 0);

    template<typename Buffer>
    inline int sendObjectTo(Buffer &buf, void *addr, socklen_t addrLen, int options = 0);

    template<typename Buffer, typename Addr>
    inline int sendObjectTo(Buffer &buf, Addr &addr, int options = 0);

    inline int sendToIgnoreWouldBlock(void *buf, int len, void *addr, socklen_t addrLen, int options = 0);

    template<typename Addr>
    inline int sendToIgnoreWouldBlock(void *buf, int len, Addr &addr, int options = 0);

    template<typename Buffer>
    inline int sendObjectToIgnoreWouldBlock(Buffer &buf, void *addr, socklen_t addrLen, int options = 0);

    template<typename Buffer, typename Addr>
    inline int sendObjectToIgnoreWouldBlock(Buffer &buf, Addr &addr, int options = 0);
};

#include "SocketImp.h"

#endif //TUNSERVER_SOCKET_H

#pragma clang diagnostic pop