//
// Created by yoni_ash on 5/29/23.
//

#ifndef TUNSERVER_BUILDER_H
#define TUNSERVER_BUILDER_H

#include "../Include.h"

using namespace std;

template<typename sock_info>
struct connection {
    socket_t socket;
    sockaddr_in address;
    sock_info info;
};

template<typename sock_info>
class Builder {
public:
    using on_error_t = ::function<void(socket_t sock, sockaddr_in remoteAddr, sock_info info, int errorNum)>;
    using on_connect_t = ::function<void(socket_t sock, sockaddr_in remoteAddr, sock_info info)>;

    Builder() = delete;

    Builder(Builder<sock_info> &) = delete;

    Builder(Builder<sock_info> &&) = delete;

    Builder &operator=(Builder<sock_info> &) = delete;

    Builder &operator=(Builder<sock_info> &&) = delete;

    explicit Builder(on_connect_t onConnect);;

    Builder(on_connect_t onConnect, on_error_t onError);

    void setOnConnect(on_connect_t onConn);

    void setOnError(on_error_t onErr);

    void setBindAddress(sockaddr_storage &addr);

    sockaddr_storage getBindAddress();

    void connect(sockaddr_in addr, sock_info info);

private:
    on_connect_t onConnect;
    on_error_t onError;
    vector<connection<sock_info>> connections{};
    socket_t maxFd{};
    fd_set writeSet{};
    fd_set errorSet{};
    bool isRunning{};
    mutex mtx{};
    sockaddr_storage bindAddr{};

    void handleConnections();
};

template<typename sock_info>
void Builder<sock_info>::setBindAddress(sockaddr_storage &addr) {
    bindAddr = addr;
}

template<typename sock_info>
sockaddr_storage Builder<sock_info>::getBindAddress() {
    return bindAddr;
}

template<typename sock_info>
Builder<sock_info>::Builder(Builder::on_connect_t onConnect) : Builder(onConnect,
                                                                       [](socket_t sock, sockaddr_in remoteAddr,
                                                                          sock_info info,
                                                                          int errorNum) {}) {}

template<typename sock_info>
Builder<sock_info>::Builder(Builder::on_connect_t onConnect, Builder::on_error_t onError) : onConnect(onConnect),
                                                                                            onError(onError) {
    if (onConnect == nullptr || onError == nullptr)throw invalid_argument("Null call back supplied");
}

template<typename sock_info>
void Builder<sock_info>::setOnConnect(Builder::on_connect_t onConn) {
    if (onConn == nullptr)throw invalid_argument("Null 'on connect' callback is disallowed");
    onConnect = onConn;
}

template<typename sock_info>
void Builder<sock_info>::setOnError(Builder::on_error_t onErr) {
    if (onErr == nullptr)throw invalid_argument("Null 'on error' callback is disallowed");
    onError = onErr;
}

template<typename sock_info>
void Builder<sock_info>::connect(sockaddr_in addr, sock_info info) {
    socket_t sock = createTcpSocket(true, true);

    if ((reinterpret_cast<sockaddr_in *>(&bindAddr)->sin_port) != 0) {
        if (bind(sock, reinterpret_cast<const sockaddr *>(&bindAddr), sizeof bindAddr) == -1)
            exitWithError("Could not bind to specified address");
    }
    if (::connect(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1 && !isConnectionInProgress() )
        exitWithError("Could not issue a connect on socket");

    connections.push_back(connection<sock_info>{sock, addr, info});
    FD_SET(sock, &writeSet);
    FD_SET(sock, &errorSet);
    maxFd = max(maxFd, sock + 1);
    unique_lock<mutex> lock(mtx);
    if (!isRunning) {
        isRunning = true;
        thread th{[this] { handleConnections(); }};
        th.detach();
    }

    lock.unlock();
}

template<typename sock_info>
void Builder<sock_info>::handleConnections() {
    timeval timeout{0, 10000};
    while (true) {
        fd_set writeCopy = writeSet;
        fd_set errorCopy = errorSet;

        /*It is asserted that there will be atleast one socket for selection*/
        int count = select(maxFd, nullptr, &writeCopy, &errorCopy, &timeout);
#ifdef _WIN32
        if (count == SOCKET_ERROR) {
            exitWithError("Could not use select on tunnel's file descriptor", WSAGetLastError());
        }
#else
        if (count == -1)exitWithError("Could not use select on tunnel's file descriptor");
#endif

        for (auto ind = connections.begin(); ind != connections.end();) {
            auto conn = *ind;
            if (FD_ISSET(conn.socket, &writeCopy)) {
                int error;
                socklen_t len = sizeof error;
                if (getsockopt(conn.socket, SOL_SOCKET, SO_ERROR, SOCKET_OPTION_POINTER_CAST &error, &len) == -1)
                    exitWithError("Could not check connect invocation's error status");
                if (error == 0)
                    onConnect(conn.socket, conn.address, conn.info);
                else onError(conn.socket, conn.address, conn.info, error);

                ind = connections.erase(ind);
            } else if (FD_ISSET(conn.socket, &errorCopy)) {
                FD_CLR(conn.socket,&writeSet);
                FD_CLR(conn.socket,&errorSet);
                onError(conn.socket, conn.address, conn.info, errno);
                CLOSE(conn.socket);
                ind = connections.erase(ind);
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
