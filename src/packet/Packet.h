//
// Created by yoni_ash on 4/22/23.
//

//#ifndef TUNSERVER_PACKET_H
//
//#define TUNSERVER_PACKET_H


#pragma once

#include "../Include.h"

class Packet {
public:
    const unsigned int MIN_SIZE = 300;

    inline explicit Packet(unsigned int size);

    inline ~Packet();

    virtual bool checkValidity() = 0;

    virtual void makeValid() = 0;

    inline void setDoFragment(bool shouldFragment);

    inline bool getDoFragment();

    inline unsigned int getSourceIp();

    inline unsigned int getDestinationIp();

    inline void setSourceIp(unsigned int addr);

    inline void setDestination(unsigned int addr);

    inline unsigned int getMaxSize() const;

    inline unsigned int getLength() const;

    inline unsigned int available() const;

    friend class Tunnel;


protected:
    unsigned char *buffer = nullptr;
    unsigned int maxSize = 0;
    unsigned int length = 0;

    inline iphdr *getIpHeader() const;
};

Packet::Packet(unsigned int size) {
    if (size < MIN_SIZE)size = MIN_SIZE;
    maxSize = size;
    buffer = new unsigned char[size]();
    length = sizeof(iphdr) + sizeof(tcphdr);

    auto iph = getIpHeader();
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = length;
    iph->id = htonl(0); // id of this packet
    iph->frag_off = 1 << 6; //do not frag
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; // correct calculation follows later
}


Packet::~Packet() {
    delete buffer;
    ::printf("Packet destroyed");
}


void Packet::setDoFragment(bool shouldFragment) {
    //TODO
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

bool Packet::getDoFragment() {
    //TODO
    return false;
}

#pragma clang diagnostic pop

unsigned int Packet::available() const {
    return maxSize - length;
}

unsigned int Packet::getSourceIp() {
    return ntohl(getIpHeader()->saddr);
}

unsigned int Packet::getDestinationIp() {
    return ntohl(getIpHeader()->daddr);
}

void Packet::setSourceIp(unsigned int addr) {
    getIpHeader()->saddr = htonl(addr);
}

void Packet::setDestination(unsigned int addr) {
    getIpHeader()->daddr = htonl(addr);
}

unsigned int Packet::getMaxSize() const {
    return maxSize;
}

unsigned int Packet::getLength() const {
    return length;
}

iphdr *Packet::getIpHeader() const {
    auto *iph = (iphdr *) buffer;
    return iph;
}
//#endif //TUNSERVER_PACKET_H
