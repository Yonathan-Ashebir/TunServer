//
// Created by yoni_ash on 6/7/23.
//
#include "../Include.h"

void test() {
    unsigned char tId[12]{};
    stun::message request(stun::message::binding_request, tId);


    // Add a SOFTWARE attribute
//    request << stun::attribute::software("software_name");

    // Add a PRIORITY attribute
//    request << stun::attribute::priority(0x6e0001fful);

    // Add a ICE-CONTROLLED attribute
//    request << stun::attribute::ice_controlled(0x932ff9b151263b36ull);

    // Add a USERNAME attribute
//    request << stun::attribute::username("username");

    // Appends a MESSAGE-INTEGRITY attribute
//    request << stun::attribute::message_integrity("key");

    // Appends a FINGERPRINT attribute as last attribute
//    request << stun::attribute::fingerprint();

    errno = 0;
    socket_t sock = createTcpSocket();
    sockaddr_in server{AF_INET, 3478};
    addrinfo hints{0, AF_INET, SOCK_STREAM}, *results;

    int code;
    if ((code = getaddrinfo("stun.sipnet.net", "http", &hints, &results)) != 0) {
        exitWithError("Could not resolve host with error " + string(gai_strerror(code)));
    }

//    server.sin_addr = *(sockaddr_in *)results->ai_addr;
    server = *(sockaddr_in *) results->ai_addr;
    server.sin_port = htons(3478);
    char buf[100];
    inet_ntop(server.sin_family, &server.sin_addr, buf, sizeof buf);
    ::printf("Stun server address is %s:%d\n", buf, ntohs(server.sin_port));

    sockaddr_in localAddress{AF_INET, htons(0), INADDR_ANY};
    if (bind(sock, reinterpret_cast<const sockaddr *>(&localAddress), sizeof localAddress) == -1)
        exitWithError("Could not bind to local port");

    if (connect(sock, reinterpret_cast<const sockaddr *>(&server), sizeof server) == -1)
        exitWithError("Could not connect to stun server");

//    char stunData[20]{0};
//    *(unsigned short *) stunData = htons(0x0001);
//    *(unsigned int *) &stunData[4] = htonl(0x2112A442);

    if (send(sock, request.data(), request.size(), 0) == -1)exitWithError("Could not send request");
    ::printf("Sent request of size: %zu\n", request.size());

    stun::message response{};

    size_t bytes = recv(sock, response.data(), response.capacity(), 0);
    if (bytes < stun::message::header_size)exitWithError("Response trimmed");
    if (response.size() > response.capacity()) {
        size_t read_bytes = response.size();
        response.resize(response.size());
        recv(sock, response.data() + bytes, read_bytes - bytes, 0);
    }

    using namespace stun::attribute;
    for (auto &attr: response) {
        // First, check the attribute type
        switch (attr.type()) {
            case type::software:
                ::printf("Software = %s\n", attr.to<type::software>().to_string().c_str());
                break;
            case type::username:
                ::printf("Username = %s\n", attr.to<type::username>().to_string().c_str());
                break;
            case type::xor_mapped_address:
                sockaddr_storage address{};
                attr.to<type::xor_mapped_address>().to_sockaddr((sockaddr *) &address);
                char addr[100];
                unsigned short port = ntohs((address.ss_family == AF_INET)
                                            ? reinterpret_cast<sockaddr_in *>(&address)->sin_port
                                            : reinterpret_cast<sockaddr_in6 *>(&address)->sin6_port);
                inet_ntop(address.ss_family,
                          (address.ss_family == AF_INET) ? (void *) &reinterpret_cast<sockaddr_in *>(&address)->sin_addr
                                                         : (void *) &reinterpret_cast<sockaddr_in6 *>(&address)->sin6_addr,
                          addr, sizeof addr);
                ::printf("Xor address = %s:%d\n", addr, port);
                break;
                // etc...
        }

    }
}

void testTcpMappedAddress() {
    auto sock = createTcpSocket();
    auto address = getTcpMappedAddress(sock);

    char addr[100];
    unsigned short port = ntohs((address->ss_family == AF_INET)
                                ? reinterpret_cast<sockaddr_in *>(address)->sin_port
                                : reinterpret_cast<sockaddr_in6 *>(address)->sin6_port);
    inet_ntop(address->ss_family,
              (address->ss_family == AF_INET) ? (void *) &reinterpret_cast<sockaddr_in *>(address)->sin_addr
                                              : (void *) &reinterpret_cast<sockaddr_in6 *>(address)->sin6_addr,
              addr, sizeof addr);
    ::printf("Mapped tcp address = %s:%d\n", addr, port);
}

int main() {
    initPlatform();
    testTcpMappedAddress();
}