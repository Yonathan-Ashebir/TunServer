//
// Created by yoni_ash on 4/22/23.
//

#ifndef TUNSERVER_PACKET_H
#define TUNSERVER_PACKET_H

#include "../Include.h"

#endif //TUNSERVER_PACKET_H


class Packet {
public:
    const unsigned int MIN_SIZE = 300;

    explicit Packet(unsigned int size);

    inline void setDoFragment(bool shouldFragment);

    inline bool getDoFragment();

    inline unsigned int getSourceIp();

    inline unsigned int getDestinationIp();

    inline void setSourceIp(unsigned int addr);

    inline void setDestination(unsigned int addr);


protected:
    unsigned char *buffer = nullptr;
    unsigned int maxSize = 0;
    unsigned int length = 0;

    inline iphdr *getIpHeader();
};
