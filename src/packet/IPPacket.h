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

class IPPacket {
public:
    const unsigned int MIN_SIZE = 300;

    IPPacket() = delete;

    IPPacket(IPPacket &old) : maxSize(old.maxSize), buffer(old.buffer), count(old.count) {
        (*old.count)++;
    };

    IPPacket(IPPacket &&old) : maxSize(old.maxSize), buffer(old.buffer), count(old.count) {
        old.buffer = nullptr;
        old.count = nullptr;
    };

    IPPacket &operator=(IPPacket &other) = delete;

    IPPacket &operator=(IPPacket &&other) = delete;

    inline explicit IPPacket(unsigned int size);

    inline ~IPPacket();

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
    unsigned int maxSize{};
    unsigned *count = new unsigned{1};

    inline iphdr *getIpHeader() const;

    inline void setLength(unsigned short len);

private:

    unsigned short length{};
};

IPPacket::IPPacket(unsigned int size) : maxSize(size) {
    if (size < MIN_SIZE)
        throw invalid_argument("capacity is " + to_string(size) + "capacity can not be below " + to_string(MIN_SIZE));

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


IPPacket::~IPPacket() {
    delete buffer;
    ::printf("Packet destroyed\n");
}


void IPPacket::setDoFragment(bool shouldFragment) {
    if (shouldFragment)
        getIpHeader()->frag_off &= ~(1 << 14);
    else
        getIpHeader()->frag_off |= 1 << 14;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"

bool IPPacket::getDoFragment() {
    return getIpHeader()->frag_off & 1 << 14;
}

#pragma clang diagnostic pop

unsigned int IPPacket::available() const {
    return maxSize - getLength();
}

unsigned int IPPacket::getSourceIp() const {
    return ntohl(getIpHeader()->saddr);
}

unsigned int IPPacket::getDestinationIp() const {
    return ntohl(getIpHeader()->daddr);
}

void IPPacket::setSourceIp(unsigned int addr) {
    getIpHeader()->saddr = htonl(addr);
}

void IPPacket::setDestination(unsigned int addr) {
    getIpHeader()->daddr = htonl(addr);
}

unsigned int IPPacket::getMaxSize() const {
    return maxSize;
}

unsigned short IPPacket::getLength() const {
    return length;
}

unsigned int IPPacket::getProtocol() const {
    return getIpHeader()->protocol;
}

iphdr *IPPacket::getIpHeader() const {
    auto *iph = (iphdr *) buffer;
    return iph;
}

unsigned short IPPacket::getIpHeaderLength() const {
    return getIpHeader()->ihl * 4;
}

void IPPacket::setLength(unsigned short len) {//warn: not check for invalid values
    getIpHeader()->tot_len = htons(len);
    length = len;
}

void IPPacket::syncWithBuffer() {
    auto iph = getIpHeader();
    length = ntohs(iph->tot_len);
}
//#endif //TUNSERVER_PACKET_H

#pragma clang diagnostic pop