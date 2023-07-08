//
// Created by yoni_ash on 6/22/23.
//

#ifndef TUNSERVER_UTILITIES_H
#define TUNSERVER_UTILITIES_H

#include "../Include.h"

struct DeferInit {
    DeferInit() = default;
};

constexpr DeferInit DEFER_INIT{};

class GAIError : public runtime_error {

public:
    GAIError(const string &msg, int err) : GAIError(msg.c_str(), err) {}

    GAIError(const char *msg, int err) : runtime_error(msg) {
        message = msg;
        message += " | code: " + to_string(err) + " | description: " + gai_strerror(err);
    }


    [[nodiscard]] const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
        return runtime_error::what();
    }

private:
    string message{};
};


inline unsigned short getPort(sockaddr_storage &addr) {
    return ntohs((addr.ss_family == AF_INET)
                 ? reinterpret_cast<sockaddr_in *>(&addr)->sin_port
                 : reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port);
}

inline string getIp(sockaddr_storage &addr) {
    string result((addr.ss_family == AF_INET) ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN, '\0');
    inet_ntop(addr.ss_family,
              (addr.ss_family == AF_INET) ? (void *) &reinterpret_cast<sockaddr_in *>(&addr)->sin_addr
                                          : (void *) &reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_addr,
              result.data(), result.size());
    int count{};
    for (char &i: std::ranges::reverse_view(result)) {
        if (i != '\0')break;
        count++;
    }
    result.erase(result.size() - count);
    return result;
}

inline string getAddressString(sockaddr_storage &addr) {
    return getIp(addr) + ":" + to_string(getPort(addr));
}

inline unique_ptr<addrinfo, function<void(addrinfo *)>>
resolveHost(const char *hostname, const char *service = "http", int family = AF_UNSPEC, int type = 0) {
    struct addrinfo hints{}, *result;
    int rv;
    hints.ai_family = family;
    hints.ai_socktype = type;
    if ((rv = getaddrinfo(hostname, service, &hints, &result)) != 0)
        throw GAIError(
                string("Could not resolve host: ") +
                hostname + " and service: " +
                service, rv);
    return unique_ptr<addrinfo, function<void(addrinfo *)>>{result, [](addrinfo *addr) { freeaddrinfo(addr); }};
}


inline bool isConnectionInProgress(int err =
#ifdef _WIN32
WSAGetLastError()
#else
        errno
#endif
) {

#ifdef _WIN32
    return err == 0 || err == WSAEWOULDBLOCK || err == WSAEINPROGRESS ||
           err == WSAEALREADY;
#else
    return err == 0 || err == EAGAIN || err == EINPROGRESS || err == EALREADY;
#endif
}

inline bool isWouldBlock(int err =
#ifdef _WIN32
WSAGetLastError()
#else
        errno
#endif
) {
#ifdef _WIN32
    return err == WSAEWOULDBLOCK;
#else
    return (err == EWOULDBLOCK || err == EAGAIN);
#endif
}

inline void initialize(sockaddr_storage &addr, sa_family_t family = AF_INET, const char *ip = nullptr, int port = -1) {
    addr.ss_family = family;
    if (port != -1) {
        if (family == AF_INET)
            reinterpret_cast<sockaddr_in * >(&addr)->sin_port = htons(port);
        else
            reinterpret_cast<sockaddr_in6 * >(&addr)->sin6_port = htons(port);
    }
    if (ip != nullptr)
        if (inet_pton(addr.ss_family, ip, (family == AF_INET) ?
                                          (char *) &reinterpret_cast<sockaddr_in * >(&addr)->sin_addr
                                                              :
                                          (char *) &reinterpret_cast<sockaddr_in6 * >(&addr)->sin6_addr) == -1)
            throw GeneralError("Could not set ip");
}

inline bool isCouldNotConnectBadNetwork(int err =
#ifdef _WIN32
WSAGetLastError()
#else
        errno
#endif
) {
#ifdef _WIN32
    return err == WSAECONNREFUSED || err == WSAETIMEDOUT || err == WSAENETUNREACH;

#else
    return err == ECONNREFUSED || err == ETIMEDOUT || err == ENETUNREACH;
#endif
}

#endif //TUNSERVER_UTILITIES_H
