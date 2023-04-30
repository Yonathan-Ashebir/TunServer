//
// Created by yoni_ash on 4/22/23.
//

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
    int fd;

};

unsigned char *Tunnel::getDataBuffer(Packet &packet) {
    return packet.buffer;
}


#endif //TUNSERVER_TUNNEL_H
