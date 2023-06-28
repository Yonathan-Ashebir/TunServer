//
// Created by yoni_ash on 6/22/23.
//

#ifndef TUNSERVER_STUN_H
#define TUNSERVER_STUN_H

#include "../Include.h"
#include "Socket.h"

inline unique_ptr<sockaddr_storage> getTCPSTUNServer(sa_family_t family = AF_INET, bool forceRefresh = false) {
    auto result = std::make_unique<sockaddr_storage>(sockaddr_storage{family});
    addrinfo hints{0, family, SOCK_STREAM}, *addresses;
    int code;
    if ((code = getaddrinfo("212.53.40.40", "http", &hints, &addresses)) != 0) {
        exitWithError("Could not resolve host with error " + string(gai_strerror(code)));
    }
    *result = *reinterpret_cast<sockaddr_storage *>(addresses->ai_addr);
    initialize(*result, family, nullptr, 3478);
    freeaddrinfo(addresses);

    return result;
}


inline unique_ptr<sockaddr_storage> getUDPSTUNServer(sa_family_t family = AF_INET, bool forceRefresh = false) {
    auto result = std::make_unique<sockaddr_storage>(sockaddr_storage{family});
    addrinfo hints{0, family, SOCK_STREAM}, *addresses;
    int code;
    if ((code = getaddrinfo("stun.l.google.com", "http", &hints, &addresses)) != 0) {
        exitWithError("Could not resolve host with error " + string(gai_strerror(code)));
    }
    *result = *reinterpret_cast<sockaddr_storage *>(addresses->ai_addr);
    initialize(*result, family, nullptr, 19302);
    freeaddrinfo(addresses);

    return result;
}

inline unique_ptr<sockaddr_storage> getTCPPublicAddress(TCPSocket &sock, bool isConnected = false) {
    if (!isConnected) {
        auto stunAddr = getTCPSTUNServer();
        sock.connect(*stunAddr);
    }

    int sent{};

//    unsigned char tsx[12];
//    stun::message request(stun::message::binding_request,
//                          tsx);
//    while (sent < request.capacity()) {
//        sent += sock.send(request.data() + sent,
//                          request.capacity() - sent);// NOLINT(cppcoreguidelines-narrowing-conversions)
//    }
//
    char testBuf[20]{};
    testBuf[1] = 1;
    while (sent < sizeof testBuf) {
        sent += sock.send(testBuf + sent,
                          sizeof testBuf - sent);// NOLINT(cppcoreguidelines-narrowing-conversions)
    }

    stun::message response{};
    int bytes{};
    while (bytes < response.capacity())
        bytes += sock.receive(response.data() + bytes,
                              response.capacity() - bytes); // NOLINT(cppcoreguidelines-narrowing-conversions)
    if (response.size() > response.capacity()) {
        response.resize(response.size());
        while (bytes < response.capacity()) {
            bytes += sock.receive(response.data() + bytes,
                                  response.capacity() - bytes); // NOLINT(cppcoreguidelines-narrowing-conversions)
        }
    }

    using namespace stun::attribute;
    auto address = std::make_unique<sockaddr_storage>(sockaddr_storage{AF_INET});//todo: compatitblity with ipv6

    for (auto &attr: response) {
        // First, check the attribute type
        switch (attr.type()) {
            case type::mapped_address:
                attr.to<type::mapped_address>().to_sockaddr((sockaddr *) address.get());
                break;
            case type::xor_mapped_address:
                attr.to<type::xor_mapped_address>().to_sockaddr((sockaddr *) address.get());
                return address;
        }
    }
    return address;
}

inline unique_ptr<sockaddr_storage> getTCPPublicAddress(sockaddr_storage &bindAddr) {
    TCPSocket sock;
    sock.setReuseAddress(true);
    sock.bind(bindAddr);
    auto result = getTCPPublicAddress(sock);
    return result;
}


inline unique_ptr<sockaddr_storage> getUDPPublicAddress(UDPSocket &sock, bool isConnected = false) {
    if (!isConnected) {
        auto stunAddr = getUDPSTUNServer();
        sock.connect(*stunAddr);
    }

//    unsigned char tsx[12];
//    stun::message request(stun::message::binding_request,
//                          tsx);
//    if (sock.send(request.data(),
//                  request.capacity()) < request.capacity())
//        throw BadException("UDP stun request trimmed");

    char testBuf[20]{};
    testBuf[1] = 1;
//    for (int ind = 4; ind < sizeof testBuf; ind++) {
//        testBuf[ind] = round(rand() *  255);
//    }
    if (sock.send(testBuf,
                  sizeof testBuf) < sizeof testBuf)
        throw BadException("UDP stun request trimmed");


    stun::message response{};
    response.resize(60);
    sock.receive(response.data(),
                 response.capacity());

    using namespace stun::attribute;
    auto address = std::make_unique<sockaddr_storage>(sockaddr_storage{AF_INET});//todo: compatitblity with ipv6
    for (auto &attr: response) {
        // First, check the attribute type
        switch (attr.type()) {
            case type::mapped_address:
                attr.to<type::mapped_address>().to_sockaddr((sockaddr *) address.get());
                break;
            case type::xor_mapped_address:
                attr.to<type::xor_mapped_address>().to_sockaddr((sockaddr *) address.get());
                return address;
        }
    }
    return address;
}


inline unique_ptr<sockaddr_storage> getUDPPublicAddress(sockaddr_storage &bindAddr) {
    UDPSocket sock;
    sock.setReuseAddress(true);
    sock.bind(bindAddr);
    auto result = getUDPPublicAddress(sock);
    return result;
}


#endif //TUNSERVER_STUN_H
