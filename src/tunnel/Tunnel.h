//
// Created by yoni_ash on 4/22/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_TUNNEL_H
#define TUNSERVER_TUNNEL_H
#pragma once

#include "../packet/TCPPacket.h"

class Tunnel {
public:
    inline explicit Tunnel(int fd) : fd(fd) {}

    virtual bool writePacket(Packet &packet) = 0;

    virtual bool readPacket(Packet &packet) = 0;

    inline int getFileDescriptor() const { return fd; }


protected:
    inline static unsigned char *getDataBuffer(Packet &packet);

    inline static void setPacketLength(Packet &packet, unsigned int len);

    int fd;

};

unsigned char *Tunnel::getDataBuffer(Packet &packet) {
    return packet.buffer;
}

void Tunnel::setPacketLength(Packet &packet, unsigned int len) {
    packet.length = len;
}


#endif //TUNSERVER_TUNNEL_H

#pragma clang diagnostic pop