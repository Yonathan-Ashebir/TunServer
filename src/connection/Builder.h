//
// Created by yoni_ash on 5/29/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_BUILDER_H
#define TUNSERVER_BUILDER_H

#include "../Include.h"

using namespace std;

template<typename SockInfo>
class Builder {
public:
    using OnError = ::function<void(TCPSocket &sock, const sockaddr_storage &remoteAddr, const SockInfo &info,
                                    int errorNum)>;
    using OnConnect = ::function<void(TCPSocket &sock, const sockaddr_storage &remoteAddr, const SockInfo &info)>;

    typedef SockInfo Info;

    Builder() = delete;

    Builder(Builder<SockInfo> &) = delete;

    Builder(Builder<SockInfo> &&) = delete;

    Builder &operator=(Builder<SockInfo> &) = delete;

    Builder &operator=(Builder<SockInfo> &&) = delete;

    explicit Builder(OnConnect onConnect);;

    Builder(OnConnect onConnect, OnError onError);

    void setOnConnect(OnConnect onConn);

    void setOnError(OnError onErr);

    void setBindPort(unsigned short port);

    unsigned short getBindPort();

    void connect(sockaddr_storage &addr, SockInfo &info, chrono::milliseconds timeout = DEFAULT_TIMEOUT);

private:
    struct Connection {
        TCPSocket socket;
        sockaddr_storage address;
        SockInfo info;
        chrono::time_point<chrono::steady_clock, chrono::nanoseconds> endTime;


        Connection(TCPSocket &socket, const sockaddr_storage &addr, const SockInfo &info,
                   const decltype(endTime) &startTime) : socket(
                socket), address(addr), info(info), endTime(startTime) {}

        Connection(const Connection &&old) noexcept: address(old.address), info(old.info), endTime(old.endTime) {
            this->socket = move(old.socket);
        }

        Connection &operator=(Connection &&old) noexcept {
            this->socket = move(old.socket);
            this->address = old.address;
            this->info = old.info;
            this->endTime = old.endTime;
            return *this;
        }
    };

    //keep temporarily
    const static OnConnect DEFAULT_ON_CONNECT;
    const static OnError DEFAULT_ON_ERROR;
    constexpr static chrono::milliseconds DEFAULT_TIMEOUT{30000};

    OnConnect onConnect;
    OnError onError;
    vector<Connection> connections{};
    socket_t maxFd{};//todo: handle well
    fd_set writeSet{};
    fd_set errorSet{};
    bool isRunning{};
    mutex mtx{};
    unsigned short bindPort{};

    void handleConnections();
};


template<typename SockInfo>
void Builder<SockInfo>::setBindPort(unsigned short port) {
    bindPort = port;
}

template<typename SockInfo>
unsigned short Builder<SockInfo>::getBindPort() {
    return bindPort;
}

template<typename SockInfo>
Builder<SockInfo>::Builder(Builder::OnConnect onConnect) : Builder(onConnect,
                                                                   DEFAULT_ON_ERROR) {}

template<typename SockInfo>
Builder<SockInfo>::Builder(Builder::OnConnect onConnect, Builder::OnError onError) : onConnect(onConnect),
                                                                                     onError(onError) {
    if (onConnect == nullptr || onError == nullptr)throw invalid_argument("A null callback supplied");
}

template<typename SockInfo>
void Builder<SockInfo>::setOnConnect(Builder::OnConnect onConn) {
    if (onConn == nullptr)throw invalid_argument("Null 'on connect' callback is disallowed");
    onConnect = onConn;
}

template<typename SockInfo>
void Builder<SockInfo>::setOnError(Builder::OnError onErr) {
    if (onErr == nullptr)throw invalid_argument("Null 'on error' callback is disallowed");
    onError = onErr;
}

template<typename SockInfo>
void Builder<SockInfo>::connect(sockaddr_storage &addr, SockInfo &info, chrono::milliseconds timeout) {
    TCPSocket sock(addr.ss_family == AF_INET6);
    sock.setBlocking(false);
    if (bindPort != 0)sock.bind(bindPort);

    if (sock.tryConnect(addr) == -1 && !isConnectionInProgress())
        throw SocketError("Could not issue a connect on socket");

    connections.push_back(move(Connection(sock, addr, info, chrono::steady_clock::now() + timeout)));
    sock.setIn(writeSet);
    sock.setIn(errorSet);
    unique_lock<mutex> lock(mtx);
    maxFd = max(maxFd, sock.getFD() + 1);
    if (!isRunning) {
        isRunning = true;
        thread th{[this] { handleConnections(); }};
        th.detach();
    }
    lock.unlock();
}

template<typename SockInfo>
void Builder<SockInfo>::handleConnections() {
    timeval timeout{0, 10000};
    while (true) {
        fd_set writeCopy = writeSet;
        fd_set errorCopy = errorSet;

        /*It is asserted that there will be atleast one socket for selection*/
        int count;
        if ((count = select(maxFd, nullptr, &writeCopy, &errorCopy, &timeout)) == -1)
            throw SocketError("Could not use select on tunnel's file descriptor");

        typedef typename decltype(connections)::iterator iterator;
        auto removeConnection = [&](const iterator &it) -> const iterator {
            auto &conn = *it;
            conn.socket.unsetFrom(writeSet);
            conn.socket.unsetFrom(errorSet);
            unique_lock<mutex> lock(mtx);
            if (maxFd == conn.socket.getFD() + 1)maxFd--;
            lock.unlock();
            return connections.erase(it);
        };

        for (auto ind = connections.begin(); ind != connections.end() && count > 0;) {
            auto &conn = *ind;
            if (conn.socket.isSetIn(writeCopy)) {
                int err = conn.socket.getLastError();
                if (err == 0) {
                    onConnect(conn.socket, conn.address, conn.info);
                    ind = removeConnection(ind);
                } else {
                    if (chrono::steady_clock::now() < conn.endTime &&
                        isCouldNotConnectBadNetwork(err)) {
                        conn.socket.connect(conn.address);
                    } else {
                        onError(conn.socket, conn.address, conn.info, err);
                        conn.socket.close();
                        ind = removeConnection(ind);
                    }
                }
                count--;
            } else if (conn.socket.isSetIn(errorCopy)) {
                int error = conn.socket.getLastError();
                onError(conn.socket, conn.address, conn.info, error);
                conn.socket.close();
                ind = removeConnection(ind);
                count--;
            } else ind++;
        }

        unique_lock<mutex> lock(mtx);
        if (connections.size() == 0) {
            isRunning = false;
            break;
        }
        lock.unlock();
    }
}


#endif //TUNSERVER_BUILDER_H

#pragma clang diagnostic pop