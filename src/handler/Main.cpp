#ifndef _WIN32

#include <csignal>

#endif

#include "../Include.h"
#include "./Handler.h"
#include "../tunnel/DatagramTunnel.h"

using namespace std;

int main() {
    initPlatform();

    socket_t tunnelFd = createUdpSocket();

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
//    inet_pton(AF_INET, "192.168.115.64", &addr.sin_addr.s_addr);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(3333);
    auto b1 = bind(tunnelFd, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b1 == -1)exitWithError("Could not bind");

    ::printf("Waiting for first packet\n");

    char buf[3072];
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in from{};
    auto r = recvfrom(tunnelFd, buf, sizeof buf, 0, reinterpret_cast<sockaddr *>(&from), &len);
    if (r == -1)exitWithError("Could not receive from socket");

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
    printf("Received %d bytes from %s:%d\n", r, ip, ntohs(from.sin_port));

    auto c = connect(tunnelFd, (sockaddr *) &from, len);
    if (c == -1)exitWithError("Could not connect socket");

//    if (fcntl(tunnelFd, F_SETFL, O_NONBLOCK) == -1) {
//        exitWithError("Could not set tunnel non-blocking");
//    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    DatagramTunnel tunnel(tunnelFd);
    Handler handler(tunnel);

    ::printf("Starting tunnel\n");
    handler.start();

}

