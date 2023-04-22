//
// Created by yoni_ash on 4/22/23.
//

#ifndef TUNSERVER_TUNNEL_H
#define TUNSERVER_TUNNEL_H

#include "../packet/TCPPacket.h"

class Tunnel {
public:
    explicit Tunnel(int fd);

    void writePacket(Packet &packet);

    void readPacket(Packet &packet);

private:
private:
    int fd;
};


#endif //TUNSERVER_TUNNEL_H
