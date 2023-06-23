//
// Created by yoni_ash on 6/21/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_SOCKET_H
#define TUNSERVER_SOCKET_H

#include "../Include.h"
#include "SocketException.h"
#include "Utilities.h"

class Socket {
public:
    inline Socket(int domain, int type, int proto);

    inline Socket(socket_t sock, int domain, int type, int proto);

    inline ~Socket();

    inline socket_t getFD();

    inline Socket(const Socket &old);

    inline Socket(Socket &&old) noexcept;

    inline Socket &operator=(const Socket &old);

    inline Socket &operator=(Socket &&old) noexcept;

    inline void setOption(int level, int option, void *val, socklen_t len);

    template<class Val>
    inline void setOption(int level, int option, Val &val);

    inline void getOption(int level, int option, void *val, socklen_t &len);

    template<class Val>
    inline void getOption(int level, int option, Val &val);

    template<class Val>
    inline void setSocketOption(int option, Val &val);

    inline void setSocketOption(int option, void *val, socklen_t len);

    inline void setLinger(linger &ln);

    template<class Val>
    inline void getSocketOption(int option, Val &val);

    inline void getSocketOption(int option, void *val, socklen_t &len);

    inline int getLastError();

    inline void setIn(fd_set &set);

    inline void unsetFrom(fd_set &set);

    inline bool isSetIn(fd_set &set);

    inline void shutdownRead();

    inline void shutdownWrite();

    inline void close();

    inline void regenerate();

protected:
    struct Data {
        socket_t socket{};
        int domain{};
        int type{};
        int protocol{};
        unsigned long count{1};
    };
    Data *data = new Data{};

private:
};

class InetSocket : public Socket {
public:

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

    inline void connect(void *addr, socklen_t len);

    template<class Addr>
    inline void connect(Addr &addr);

    inline void connect(const char *ip, unsigned short port);

    inline void connect(const string &ip, unsigned short port);

    inline void connectToHost(const char *hostname, unsigned short port);

    inline int trySend(const void *buf, int len, int options = 0);

    inline int send(const void *buf, int len, int options = 0);

    inline int tryReceive(void *buf, int len, int options = 0);

    inline int receive(void *buf, int len, int options = 0);
};


class Inet6Socket : public Socket {
};

class TCPSocket : public InetSocket {
public:
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

    inline TCPSocket accept(void *addr, socklen_t &len);

    template<class Addr>
    inline TCPSocket accept(Addr addr);

};


class UDPSocket : public InetSocket {
    inline explicit UDPSocket(bool ipv6 = false);

    template<class Val>
    inline void setUDPOption(int option, Val &val);

    inline void setUDPOption(int option, void *val, int len);

    inline void getUDPOption(int option, void *val, socklen_t &len);

    template<class Val>
    inline void getUDPOption(int option, Val &val);
};

#include "SocketImp.h"

#endif //TUNSERVER_SOCKET_H

#pragma clang diagnostic pop