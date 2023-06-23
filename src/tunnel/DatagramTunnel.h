//
// Created by yoni_ash on 4/29/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#ifndef TUNSERVER_DATAGRAMTUNNEL_H
#define TUNSERVER_DATAGRAMTUNNEL_H
#pragma once

#include "../Include.h"

#include "Tunnel.h"

class DatagramTunnel : public Tunnel {
public:
    explicit DatagramTunnel(UDPSocket sock) : Tunnel(sock) {};

    inline bool writePacket(IPPacket &packet) override;

    inline bool readPacket(IPPacket &packet) override;

protected:
    inline UDPSocket &getDatagramSocket() {
        return *reinterpret_cast<UDPSocket * >(&sock);
    }
};

bool DatagramTunnel::writePacket(IPPacket &packet) {
    packet.validate();
    auto buffer = getDataBuffer(packet);
    auto sock = getDatagramSocket();
    auto res = sock.sendIgnoreWouldBlock(buffer, packet.getLength());
    return res == packet.getLength();
}

bool DatagramTunnel::readPacket(IPPacket &packet) {
    auto len = packet.getMaxSize();
    auto buffer = getDataBuffer(packet);
    auto sock = getDatagramSocket();
    auto total = sock.receiveIgnoreWouldBlock(buffer, len); // NOLINT(cppcoreguidelines-narrowing-conversions)
    packet.syncWithBuffer();
    if (total == len) { printError("Might have dropped something"); }
    if (!packet.isValid()) return false;
    else return true;
}

#pragma clang diagnostic pop


#endif //TUNSERVER_DATAGRAMTUNNEL_H
