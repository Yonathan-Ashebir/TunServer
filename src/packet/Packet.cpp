//
// Created by yoni_ash on 4/22/23.
//

#include "Packet.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

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

iphdr *Packet::getIpHeader() {
    auto *iph = (iphdr *) buffer;
    return iph;
}

void Packet::setDoFragment(bool shouldFragment) {
    //TODO
}

bool Packet::getDoFragment() {
    //TODO
}

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

Packet::~Packet() {
    delete buffer;
    ::printf("Packet destroyed");

}

unsigned int Packet::getMaxSize() const {
    return maxSize;
}

unsigned int Packet::getLength() {
    return length;
}


#pragma clang diagnostic pop