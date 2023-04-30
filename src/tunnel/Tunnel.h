//
// Created by yoni_ash on 4/22/23.
//

#ifndef TUNSERVER_TUNNEL_H
#define TUNSERVER_TUNNEL_H
#pragma once

#include "../packet/TCPPacket.h"

class Tunnel {
public:
    explicit Tunnel(int fd);

    virtual bool writePacket(Packet &packet);

    virtual bool readPacket(Packet &packet);

    inline int getFileDescriptor() const;


protected:
    unsigned char *getDataBuffer(Packet &packet);
    int fd;

};


#endif //TUNSERVER_TUNNEL_H
