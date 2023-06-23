//
// Created by yoni_ash on 4/22/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_TUNNEL_H
#define TUNSERVER_TUNNEL_H

#include "../packet/TCPPacket.h"

class Tunnel {
public:
    inline explicit Tunnel(Socket& fd) : sock(fd) {}

    virtual bool writePacket(IPPacket &packet) = 0;

    virtual bool readPacket(IPPacket &packet) = 0;

    inline Socket getSocket() const { return sock; }


protected:
    inline static unsigned char *getDataBuffer(IPPacket &packet);

    inline static void setPacketLength(IPPacket &packet, unsigned int len);

    Socket sock;
};

unsigned char *Tunnel::getDataBuffer(IPPacket &packet) {
    return packet.buffer;
}

void Tunnel::setPacketLength(IPPacket &packet, unsigned int len) {
    packet.length = len;
}


#endif //TUNSERVER_TUNNEL_H

#pragma clang diagnostic pop