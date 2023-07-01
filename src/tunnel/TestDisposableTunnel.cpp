//
// Created by DELL on 7/1/2023.
//

#include "DisposableTunnel.h"
#include "../packet/CompressedTCPPacket.h"

void test1() {
    TCPSocket sock1;
    TCPSocket server;
    sock1.setReuseAddress(true);
    server.setReuseAddress(true);
    server.bind(0);
    server.listen();

    sockaddr_storage serverAddr{};
    server.getBindAddress(serverAddr);
    initialize(serverAddr, AF_INET, "127.0.0.1", -1);

    /*some demo data*/
    CompressedTCPPacket tcpPacket{};
    tcpPacket.makeSyn(1, 1);
    tcpPacket.setWindowScale(2);
    tcpPacket.setWindowSize(numeric_limits<unsigned short>::max());

    unsigned short tunnelIdToDelete = 13;
    sockaddr_storage newTunnelAddr{AF_INET};
    auto addrIn = reinterpret_cast<sockaddr_in *>(&newTunnelAddr);
    unsigned int newTunnelResponseIp = 2341341234;
    addrIn->sin_port = toNetworkByteOrder((unsigned short) 1234);
    memcpy(&addrIn->sin_addr, &newTunnelResponseIp, sizeof newTunnelResponseIp);
    /*send the data*/
    thread th{[&] {
        sock1.connect(serverAddr);
        fd_set rcv, snd, err;
        FD_ZERO(&rcv);
        FD_ZERO(&snd);
        FD_ZERO(&err);
//        sock1.sendObject(1);
        DisposableTunnel tunnel{sock1, rcv, snd, err};
        for (int ind = 0; ind < 300; ind++) {
            usleep(10000);
            tunnel.writePacket(tcpPacket);
            tunnel.writePacket(tcpPacket);
            tunnel.writePacket(tcpPacket);
            tunnel.sendDeleteTunnelRequest(tunnelIdToDelete);
            tunnel.sendNewTunnelRequest();
            tunnel.sendNewTunnelResponse(newTunnelAddr);
            tunnel.flushPackets();
        }
    }};

    /* receive it*/
    sockaddr_storage addr{};
    auto sock2 = server.accept(addr);

    fd_set rcv, snd, err;
    FD_ZERO(&rcv);
    FD_ZERO(&snd);
    FD_ZERO(&err);
    DisposableTunnel tunnel{sock2, rcv, snd, err};
    tunnel.setId(1);
    tunnel.setOnDeleteTunnelRequest([&](unsigned id) {
        cout << "Deleted tunnel with id " << id << endl;
    });

    tunnel.setOnNewTunnelResponse([&](sockaddr_storage &addr) {
        cout << "Accepted new tunnel response" << endl;
    });

    tunnel.setOnNewTunnelRequest([] {
        cout << "Received new tunnel request" << endl;
    });

//    timeval timeout{};
    sock2.setBlocking(false);
    unsigned int packetCount{};
    while (true) {
        fd_set rcvCopy = rcv, sndCopy = snd, errCopy = err;
        int count = select(sock2.getFD() + 1, &rcvCopy, &sndCopy, &errCopy, nullptr);
        if (count < 0)throw SocketException("Could not select on sock2");
        if (sock2.isSetIn(rcvCopy)) {
            auto packets = tunnel.readPackets();
            if (!packets.empty()) {
                cout << "Received " << packets.size() << " packets, total: " << (packetCount += packets.size()) << endl;
            }
        }
    }
    th.join();
}

int main() {
    initPlatform();
    cout << "Some thing" << endl;
    test1();
}