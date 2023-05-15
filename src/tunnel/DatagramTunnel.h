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
    explicit DatagramTunnel(socket_t fd);

    inline bool writePacket(Packet &packet) override;

    inline bool readPacket(Packet &packet) override;

};

bool DatagramTunnel::writePacket(Packet &packet) {
    packet.validate();
    auto buffer = getDataBuffer(packet);
    auto res = send(fd,(char *) buffer, packet.getLength(), 0);
    if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        exitWithError("Could not write a packet");
    }
    return res == packet.getLength();
}

bool DatagramTunnel::readPacket(Packet &packet) {
    auto len = packet.getMaxSize();
    auto buffer = getDataBuffer(packet);
    auto total = recv(fd, (char *)buffer, len, 0);

    if (total == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        exitWithError("Could not read a packet");
    }
    packet.syncWithBuffer();
    if (total == len) { printError("Might have dropped something"); }
    if (!packet.isValid()) return false;
    else return true;
}

#pragma clang diagnostic pop


#endif //TUNSERVER_DATAGRAMTUNNEL_H
