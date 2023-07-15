#ifndef _WIN32

#include <csignal>

#endif

#include "../Include.h"
#include "Handler.h"
#include <tun-commons/tunnel/DatagramTunnel.h>

using namespace std;

int main() {
    initPlatform();

    UDPSocket tunnelSocket;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
//    inet_pton(AF_INET, "192.168.115.64", &addr.sin_addr.s_addr);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(3333);
    tunnelSocket.bind(addr);
    ::printf("Waiting for first packet\n");

    char buf[3072];
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in from{};
    auto r = tunnelSocket.receiveFrom(buf, sizeof buf, from);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
    printf("Received %d bytes from %s:%d\n", r, ip, ntohs(from.sin_port));

    tunnelSocket.connect(from);

//    if (fcntl(tunnelSocket, F_SETFL, O_NONBLOCK) == -1) {
//        exitWithError("Could not set tunnel non-blocking");
//    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    DatagramTunnel tunnel(tunnelSocket);
    Handler handler(tunnel);

    ::printf("Starting tunnel\n");
    handler.start();

}

