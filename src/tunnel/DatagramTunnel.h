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

    bool writePacket(Packet &packet);

    bool readPacket(Packet &packet);

};


#pragma clang diagnostic pop


#endif //TUNSERVER_DATAGRAMTUNNEL_H
