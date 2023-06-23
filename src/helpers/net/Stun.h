//
// Created by yoni_ash on 6/22/23.
//

#ifndef TUNSERVER_STUN_H
#define TUNSERVER_STUN_H

#include "../Include.h"
#include "Socket.h"

inline unique_ptr<sockaddr_storage> getTUNServerAddress(bool forceRefresh = false) {
    auto result = std::make_unique<sockaddr_storage>(sockaddr_storage{AF_INET6});
    auto result_in = reinterpret_cast<sockaddr_in *>(result.get());//todo in

    addrinfo hints{0, AF_INET, SOCK_STREAM}, *addresses;
    int code;
    if ((code = getaddrinfo("stun.sipnet.net", "http", &hints, &addresses)) != 0) {
        exitWithError("Could not resolve host with error " + string(gai_strerror(code)));
    }
    *result = *reinterpret_cast<sockaddr_storage *>(addresses->ai_addr);
    result_in->sin_port = htons(3478);
    freeaddrinfo(addresses);

    return result;
}

inline unique_ptr<sockaddr_storage> getTCPMappedAddress(TCPSocket &sock, bool isConnected = false) {
    if (!isConnected) {
        auto stunAddr = getTUNServerAddress();
        sock.connect(*stunAddr);
    }

    unsigned char tsx[12];
    stun::message request(stun::message::binding_request,
                          tsx);

//    char testBuf[20]{};
//    testBuf[1] = 1;
    int sent{};
    while (sent < request.size()) {
        sent += sock.send(request.data() + sent,
                          request.size() - sent);// NOLINT(cppcoreguidelines-narrowing-conversions)
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

inline unique_ptr<sockaddr_storage> getTCPMappedAddress(sockaddr_storage &bindAddr) {
    TCPSocket sock;
    sock.setReuseAddress(true);
    sock.bind(bindAddr);
    auto result = getTCPMappedAddress(sock);
    return result;
}

#endif //TUNSERVER_STUN_H
