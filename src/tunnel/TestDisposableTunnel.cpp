//
// Created by DELL on 7/1/2023.
//

#include <cassert>
#include "DisposableTunnel.h"
#include "../packet/CompressedTCPPacket.h"

void testSendAndReceive() {
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
    tcpPacket.setProtocol(1);
//    tcpPacket.makeSyn(1, 1);
//    tcpPacket.setWindowScale(2);
    tcpPacket.setWindow(numeric_limits<unsigned short>::max());
    SmartBuffer buf{100};
    buf.put((int)1);
    buf.put((long)2);
    buf.put((double)2.321);
    tcpPacket.setTCPPayload(buf.as(0, 100));

    unsigned short tunnelIdToDelete = 13;

    sockaddr_storage newTunnelAddr{AF_INET};
    auto addrIn = reinterpret_cast<sockaddr_in *>(&newTunnelAddr);
    unsigned int newTunnelResponseIp = 2341341234;
    addrIn->sin_port = toNetworkByteOrder((unsigned short) 1234);
    memcpy(&addrIn->sin_addr, &newTunnelResponseIp, sizeof newTunnelResponseIp);

    /*send the data*/
    unsigned int loopCount = 30000;
    chrono::milliseconds testTimeout{10000};
    thread th{[&] {
        sock1.connect(serverAddr);
        fd_set rcv, snd, err;
        FD_ZERO(&rcv);
        FD_ZERO(&snd);
        FD_ZERO(&err);
        DisposableTunnel senderTunnel{0,sock1, rcv, snd, err};

        TCPSocket sock3;
        TCPSocket sock4;
        sock4.bind(0);
        sock4.listen();
        sockaddr_storage sock4Addr{};
        sock4.getBindAddress(sock4Addr);
//        sock3.connectToHost("212.53.40.40", 80);
        initialize(sock4Addr, AF_INET, "127.0.0.1", -1);
        sock3.connect(sock4Addr);
        sock3.setBlocking(false);
        DisposableTunnel tunnelToClose{0,sock3, rcv, snd, err};
        tunnelToClose.setOnUnsentData([&](DisposableTunnel::UnsentData &unsentData) {
#ifdef LOGGING
            cout << "UnsentData.offset: " << unsentData.offset << " remaining: " << unsentData.remaining << endl;
#endif
            senderTunnel.sendUnsentData(unsentData);
        });

        for (int ind = 0; ind < 30; ind++) {
            if (!tunnelToClose.isOpen())
                break;
            this_thread::sleep_for(chrono::milliseconds(1));
            tunnelToClose.writePacket(tcpPacket);
            tunnelToClose.sendDeleteTunnelRequest(tunnelIdToDelete);
            tunnelToClose.writePacket(tcpPacket);
            tunnelToClose.sendNewTunnelRequest();
            tunnelToClose.writePacket(tcpPacket);
            tunnelToClose.sendNewTunnelResponse(newTunnelAddr);
        }
        tunnelToClose.close();

//        sock1.setBlocking(false);
        for (int ind = 0; ind < loopCount; ind++) {
//              this_thread::sleep_for(chrono::nanoseconds(10000));
            senderTunnel.writePacket(tcpPacket);
            senderTunnel.sendDeleteTunnelRequest(tunnelIdToDelete);
            senderTunnel.writePacket(tcpPacket);
            senderTunnel.sendNewTunnelRequest();
            senderTunnel.writePacket(tcpPacket);
            senderTunnel.sendNewTunnelResponse(newTunnelAddr);
            senderTunnel.checkStatus();
        }
        senderTunnel.flushPackets();
    }};

    /* receive it*/
    sockaddr_storage addr{};
    auto sock2 = server.accept(addr);

    fd_set rcv, snd, err;
    FD_ZERO(&rcv);
    FD_ZERO(&snd);
    FD_ZERO(&err);
    DisposableTunnel tunnel{1,sock2, rcv, snd, err};
    tunnel.setOnDeleteTunnelRequest([&](unsigned id) {
        assert(id == 13);
#ifdef LOGGING
        cout << "Deleted the tunnel with id " << id << endl;
#endif
    });

    tunnel.setOnNewTunnelResponse([&](sockaddr_storage &addr) {
        assert(memcmp(&addr, &newTunnelAddr, sizeof(sockaddr_storage)) == 0);
#ifdef LOGGING
        cout << "Accepted new tunnel response" << endl;
#endif
    });

    tunnel.setOnNewTunnelRequest([] {
#ifdef LOGGING
        cout << "Received new tunnel request" << endl;
#endif
    });

//    timeval timeout{};
    auto started = chrono::steady_clock::now();
    sock2.setBlocking(false);
    unsigned int packetCount{};
    timeval timeout{0, 10000};
    while (true) {
        fd_set rcvCopy = rcv, sndCopy = snd, errCopy = err;
        timeval cpyTimeout = timeout;
        int count = select((int) sock2.getFD() + 1, &rcvCopy, &sndCopy, &errCopy, &cpyTimeout);
        if (count < 0)throw SocketError("Could not select on sock2");
        if (chrono::steady_clock::now() - started > testTimeout)
            throw logic_error("Test timed out before receiving all packets");
        if (count == 0)continue;
        if (sock2.isSetIn(rcvCopy)) {
            auto packets = tunnel.readPackets();
            packetCount += packets.size();
#ifdef LOGGING
            if (!packets.empty()) cout << "Received " << packets.size() << " packets, total: " << packetCount << endl;
#endif
            for (auto &p: packets) {
                assert(p.getLength() == tcpPacket.getLength());
                assert(memcmp(p.getBuffer(), tcpPacket.getBuffer(), tcpPacket.getPayloadOffset()) == 0);
                tcpPacket.getPayload().manipulate([&](char *buf, unsigned int len) -> unsigned int {
                    static auto off = 0;
                    assert(memcmp(
                            p.getBuffer() + reinterpret_cast<CompressedTCPPacket *>(&p)->getTCPPayloadOffset() + off,
                            buf, len) == 0);
                    off += len;
                    if (off == tcpPacket.getPayload().getLimit()) off = 0;
                    return len;
                }, 0);
            }
            if (packetCount >= loopCount * 3)break;
        }
    }
    th.join();
    cout << "Test send and receive passed" << endl;
}

int main() {
    initPlatform();
    testSendAndReceive();
}