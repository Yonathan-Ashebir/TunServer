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
    using OnError = function<void(int errorNum, const sockaddr_storage &remoteAddr, const SockInfo &info
    )>;
    using OnConnect = function<void(TCPSocket &sock, const sockaddr_storage &remoteAddr, const SockInfo &info)>;

    typedef SockInfo Info;

    explicit Builder(OnConnect onConnect, OnError onError = EMPTY_ON_ERROR);

    Builder() = delete;

    Builder(Builder<SockInfo> &) = default;

    Builder(Builder<SockInfo> &&) noexcept = default;

    Builder &operator=(const Builder<SockInfo> &) = default;

    Builder &operator=(Builder<SockInfo> &&) noexcept = default;

    void setDefaultOnConnect(OnConnect onConn);

    void setDefaultOnError(OnError onErr);

    void setBindPort(unsigned short port);

    unsigned short getBindPort();

    void connect(sockaddr_storage &addr, SockInfo &info, OnConnect onConnect = {}, OnError onError = {},
                 chrono::milliseconds timeout = DEFAULT_TIMEOUT);

private:
    struct Connection {
        TCPSocket socket{DEFER_INIT};
        sockaddr_storage address;
        SockInfo info;

        OnConnect onConnect;
        OnError onError;
        chrono::steady_clock::time_point endTime;


        Connection(TCPSocket &socket, const sockaddr_storage &addr, const SockInfo &info, OnConnect onConnect,
                   OnError onError,
                   chrono::steady_clock::time_point startTime) : socket(
                socket), address(addr), info(info), onConnect(onConnect), onError(onError), endTime(startTime) {}
    };

    struct Data {
        const OnConnect DEFAULT_ON_CONNECT{
                [&](TCPSocket &sock, const sockaddr_storage &addr, const SockInfo &info) {
                    onConnect(sock, addr, info);
                }};
        const OnError DEFAULT_ON_ERROR{
                [&](int err, const sockaddr_storage &addr, const SockInfo &info) {
                    onError(err, addr, info);
                }};

        OnConnect onConnect;
        OnError onError;
        vector<Connection> connections{};
        socket_t maxFd{};//todo: handle well
        fd_set fdSet{};
        bool isRunning{};
        mutex mtx{};
        unsigned short bindPort{};

        Data(OnConnect onConnect, OnError onError) : onConnect(onConnect), onError(onError) {
        }
    };

    //keep temporarily
    constexpr static auto EMPTY_ON_ERROR = [](int, const sockaddr_storage &addr, const SockInfo &info) {};
    constexpr static chrono::milliseconds DEFAULT_TIMEOUT{30000};

    shared_ptr<Data> data;


    void handleConnections();
};


template<typename SockInfo>
void Builder<SockInfo>::setBindPort(unsigned short port) {
    data->bindPort = port;
}

template<typename SockInfo>
unsigned short Builder<SockInfo>::getBindPort() {
    return data->bindPort;
}

template<typename SockInfo>
Builder<SockInfo>::Builder(Builder::OnConnect onConnect, Builder::OnError onError) :data(new Data{onConnect, onError}) {
    if (!onConnect)throw invalid_argument("Empty onConnect callback is disallowed");
    if (!onError)throw invalid_argument("Empty onError callback is disallowed");
}

template<typename SockInfo>
void Builder<SockInfo>::setDefaultOnConnect(Builder::OnConnect onConn) {
    if (!onConn)throw invalid_argument("Empty onConnect callback is disallowed");
    data->onConnect = onConn;
}

template<typename SockInfo>
void Builder<SockInfo>::setDefaultOnError(Builder::OnError onErr) {
    if (!onErr)throw invalid_argument("Empty onError callback is disallowed");
    data->onError = onErr;
}

template<typename SockInfo>
void Builder<SockInfo>::connect(sockaddr_storage &addr, SockInfo &info, OnConnect onConnect, OnError onError,
                                chrono::milliseconds timeout) {
    if (!onConnect)onConnect = data->DEFAULT_ON_CONNECT;
    if (!onError)onError = data->DEFAULT_ON_ERROR;

    TCPSocket sock(addr.ss_family == AF_INET6);
    sock.setBlocking(false);
    if (data->bindPort != 0)sock.bind(data->bindPort);
    try {
        sock.connect(addr, true);
        data->connections.push_back(
                move(Connection(sock, addr, info, onConnect, onError, chrono::steady_clock::now() + timeout)));
        sock.setIn(data->fdSet);
        unique_lock<mutex> lock(data->mtx);
        data->maxFd = max(data->maxFd, sock.getFD() + 1);
        if (!data->isRunning) {
            data->isRunning = true;
            thread th{[this] { handleConnections(); }};
            th.detach();
        }
        lock.unlock();
    } catch (SocketError &e) {
        onError(sock.getLastError(), addr, info);
    }
}

template<typename SockInfo>
void Builder<SockInfo>::handleConnections() {
    timeval timeout{0, 10000};
    while (true) {
        fd_set writeCopy = data->fdSet;
        fd_set errorCopy = data->fdSet;

        /*It is asserted that there will be atleast one socket for selection*/
        int count;
        if ((count = select(data->maxFd, nullptr, &writeCopy, &errorCopy, &timeout)) == -1)
            throw SocketError("Could not use select on tunnel's file descriptor");

        typedef typename decltype(data->connections)::iterator iterator;
        auto removeConnection = [&](const iterator &it) -> iterator {
            auto &conn = *it;
            conn.socket.unsetFrom(data->fdSet);
            unique_lock<mutex> lock(data->mtx);
            if (data->maxFd == conn.socket.getFD() + 1)data->maxFd--;
            /*todo: check for necessity to loop through it all in order to set maxFD*/
            auto res = data->connections.erase(it);
            return res;
        };

        for (auto ind = data->connections.begin(); ind != data->connections.end() && count > 0;) {
            auto &conn = *ind;
            if (conn.socket.isSetIn(writeCopy)) {
                int err = conn.socket.getLastError();
                if (err == 0) {
                    conn.onConnect(conn.socket, conn.address, conn.info);
                    ind = removeConnection(ind);
                } else {
                    if (chrono::steady_clock::now() < conn.endTime &&
                        isCouldNotConnectBadNetwork(err)) {
                        conn.socket.connect(conn.address);
                    } else {
                        conn.onError(err, conn.address, conn.info);
                        conn.socket.close();
                        ind = removeConnection(ind);
                    }
                }
                count--;
            } else if (conn.socket.isSetIn(errorCopy)) {
                int error = conn.socket.getLastError();
                conn.onError(error, conn.address, conn.info);
                conn.socket.close();
                ind = removeConnection(ind);
                count--;
            } else ind++;
        }

        unique_lock<mutex> lock(data->mtx);
        if (data->connections.size() == 0) {
            data->isRunning = false;
            break;
        }
        lock.unlock();
    }
}


#endif //TUNSERVER_BUILDER_H

#pragma clang diagnostic pop