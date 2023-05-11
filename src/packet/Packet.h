//
// Created by yoni_ash on 4/22/23.
//

//#ifndef TUNSERVER_PACKET_H
//
//#define TUNSERVER_PACKET_H


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma once

#include "../Include.h"

#define DEFAULT_MSS 576

class Packet {
public:
    const unsigned int MIN_SIZE = 300;

    Packet()= delete;
    Packet(Packet&)= delete;
    Packet(Packet&&)= delete;


    inline explicit Packet(unsigned int size);

    inline ~Packet();

    virtual bool isValid() = 0;

    virtual void validate() = 0;

    inline void setDoFragment(bool shouldFragment);

    inline bool getDoFragment();

    inline unsigned int getSourceIp() const;

    inline unsigned int getDestinationIp() const;

    inline void setSourceIp(unsigned int addr);

    inline void setDestination(unsigned int addr);

    inline unsigned int getMaxSize() const;

    inline unsigned short getLength() const;

    inline unsigned int available() const;

    friend class Tunnel;

    inline unsigned int getProtocol() const;

    inline unsigned short getIpHeaderLength() const;

    inline virtual void syncWithBuffer();
protected:
    unsigned char *buffer{};

    const unsigned int maxSize{};

    inline iphdr *getIpHeader() const;

    inline void setLength(unsigned short len);

private:

    unsigned short length{};
};

Packet::Packet(unsigned int size) : maxSize(size) {
    if (size < MIN_SIZE)
        throw invalid_argument("size is " + to_string(size) + "size can not be below " + to_string(MIN_SIZE));

    buffer = new unsigned char[size]();

    auto iph = getIpHeader();
    setLength(sizeof(iphdr));
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->frag_off = 0;//1 << 6; //do not frag
    iph->ttl = 64;
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
    return maxSize - getLength();
}

unsigned int Packet::getSourceIp() const {
    return ntohl(getIpHeader()->saddr);
}

unsigned int Packet::getDestinationIp() const {
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

unsigned short Packet::getLength() const {
    return length;
}

unsigned int Packet::getProtocol() const {
    return getIpHeader()->protocol;
}

iphdr *Packet::getIpHeader() const {
    auto *iph = (iphdr *) buffer;
    return iph;
}

unsigned short Packet::getIpHeaderLength() const {
    return getIpHeader()->ihl * 4;
}

void Packet::setLength(unsigned short len) {//warn: not check for invalid values
    getIpHeader()->tot_len = htons(len);
    length = len;
}

void Packet::syncWithBuffer() {
    auto iph = getIpHeader();
    length = ntohs(iph->tot_len);
}
//#endif //TUNSERVER_PACKET_H

#pragma clang diagnostic pop