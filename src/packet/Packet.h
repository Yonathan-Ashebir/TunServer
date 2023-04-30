//
// Created by yoni_ash on 4/22/23.
//

#ifndef TUNSERVER_PACKET_H

#define TUNSERVER_PACKET_H

#pragma once

#include "../Include.h"

#include "../tunnel/Tunnel.h"

class Packet {
public:
    const unsigned int MIN_SIZE = 300;

    explicit Packet(unsigned int size);

    ~Packet();

    inline virtual bool checkValidity() = 0;

    inline virtual void makeValid() = 0;

    inline void setDoFragment(bool shouldFragment);

    inline bool getDoFragment();

    inline unsigned int getSourceIp();

    inline unsigned int getDestinationIp();

    inline void setSourceIp(unsigned int addr);

    inline void setDestination(unsigned int addr);

    inline unsigned int getMaxSize() const;

    inline unsigned int getLength();

    inline unsigned int available() const;

    friend class Tunnel;


protected:
    unsigned char *buffer = nullptr;
    unsigned int maxSize = 0;
    unsigned int length = 0;

    inline iphdr *getIpHeader();
};

#endif //TUNSERVER_PACKET_H
