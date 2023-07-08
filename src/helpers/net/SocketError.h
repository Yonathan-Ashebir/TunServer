//
// Created by yoni_ash on 6/21/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_SOCKETERROR_H
#define TUNSERVER_SOCKETERROR_H

#include "../Include.h"
#include <system_error>

/*enum class socket_errc {
    success = 0
};
namespace std {
    template<>
    struct is_error_condition_enum<socket_errc> : public true_type {
    };
}

class socket_category : public error_category {
public:
    virtual const char *name() const noexcept {
        return "socket";
    }

    virtual error_condition default_error_condition(int ev) const noexcept {
        switch (ev) {
            case 0:
                return error_condition(socket_errc::success);
            default:
                return generic_category().default_error_condition(ev);
        }

    }

    virtual bool equivalent(const error_code &code, int condition) const noexcept {
        return *this == code.category() &&
               static_cast<int>(default_error_condition(code.value()).value()) == condition;
    }

    virtual string message(int ev) const {
        switch (ev) {
            case 200:
                return "OK";
            case 403:
                return "403 Forbidden";
            case 404:
                return "404 Not Found";
            case 500:
                return "500 Internal Server Error";
            case 503:
                return "503 Service Unavailable";
            default:
                return "Unknown error";
        }
    }
}

        socket_category;

error_condition make_error_condition(socket_errc e) {
    return error_condition(static_cast<int>(e), socket_category);
}*/

class SocketError : public runtime_error {
public:


    SocketError(const string &msg, int err = getLastSocketError()) : SocketError(msg.c_str(), err) {}

    SocketError(const char *msg, int err = getLastSocketError()) : runtime_error(msg) {
        message = msg;
        message += " | code: " + to_string(err) + " | description: " + getErrorDescription(err);
    }


    ~SocketError() _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {

    }

    const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
        return message.c_str();
    }

    string message{};
protected:
    static int getLastSocketError() {
#ifdef _WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }

    static string getErrorDescription(int err = getLastSocketError()) {
#ifdef _WIN32
        char *buf{};
        auto count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                   nullptr, err, 0, buf, 0,
                                   nullptr);
        if (count == 0) return "Could not format socket error description";
        string result(buf);
        LocalFree(buf);
        return result;
#else
        return string(strerror(err));
#endif
    }
};


#endif //TUNSERVER_SOCKETERROR_H

#pragma clang diagnostic pop