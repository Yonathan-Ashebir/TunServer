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
    explicit DatagramTunnel(int fd);

    inline bool writePacket(Packet &packet) override;

    inline bool readPacket(Packet &packet) override;

};

bool DatagramTunnel::writePacket(Packet &packet) {
    auto len = packet.getMaxSize();
    auto buffer = getDataBuffer(packet);
    auto res = send(fd, buffer, packet.getLength(), 0);
    return res == 0;
}

bool DatagramTunnel::readPacket(Packet &packet) {
    auto len = packet.getMaxSize();
    auto buffer = getDataBuffer(packet);
    auto total = recv(fd, buffer, len, 0);
    if (total == len) { printError("Might have dropped something"); }
    if (!packet.checkValidity()) return false;
    else return true;
}

#pragma clang diagnostic pop


#endif //TUNSERVER_DATAGRAMTUNNEL_H
