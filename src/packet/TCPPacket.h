//
// Created by yoni_ash on 4/21/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_PACKET_H
#define TUNSERVER_PACKET_H


#pragma once

#include "../Include.h"
#include "Packet.h"

constexpr unsigned int OPTIONS_INDEX = sizeof(iphdr) + sizeof(tcphdr);

class TCPPacket : public Packet {
public:

    explicit TCPPacket(unsigned int size);

    inline void setEnds(sockaddr_in &src, sockaddr_in &dest);

    inline void setSource(sockaddr_in &src);

    inline void setDestination(sockaddr_in &dest);


    inline void setSequenceNumber(unsigned int seq);

    inline void setAcknowledgmentNumber(unsigned int ack);

    inline void setUrgentPointer(unsigned short urg);

    inline void setSynFlag(bool isSyn);

//   inline void setAckFlag(bool isSyn);

//    inline void setUrgentFlag(bool isUrg);

    inline void setFinFlag(bool isFin);

    inline void setResetFlag(bool isRst);

    inline void setPushFlag(bool isPush);

    inline void setWindowSize(unsigned short window);

    inline void setWindowScale(unsigned char shift);

    inline void setWindowSize(unsigned short window, unsigned char shift);

    inline void setMSS(unsigned short size);

    void setOption(unsigned char kind, unsigned char len, unsigned char *payload);

    bool removeOption(unsigned char kind);

    inline sockaddr_in getSource();

    inline sockaddr_in getDestination();

    inline bool isFrom(sockaddr_in &src);

    inline bool isTo(sockaddr_in &dest);

    inline unsigned int getSequenceNumber();

    inline unsigned int getAcknowledgmentNumber();

    inline unsigned short getUrgentPointer();

    inline bool isSyn() const;

    inline bool isFin() const;

    inline bool isReset() const;

    inline bool isPush() const;

    inline bool isAck() const;

    inline bool isUrg() const;

    inline unsigned short getWindowSize();

    inline unsigned char getWindowShift();

    inline unsigned short getMSS();

    unsigned char getOption(unsigned char kind, unsigned char *data, unsigned char len);

    inline unsigned int appendData(unsigned char *data, unsigned int len);

    inline void clearData();

    inline unsigned int getDataLength() const;

    inline unsigned int copyDataTo(unsigned char *buff, unsigned int len);

    inline void makeSyn(unsigned int seq, unsigned int ack);

    inline void makeResetSeq(unsigned int seq);

    inline void makeResetAck(unsigned int ack);

    inline void makeFin(unsigned int seq, unsigned int ack);

    inline void makeNormal(unsigned int seqNo, unsigned int ackSeq);

    inline void swapEnds();

    inline unsigned int getSegmentLength() const;

    void makeValid() override;

    bool checkValidity() override;

private:

    unsigned int optionsLength = 0;


    inline tcphdr *getTcpHeader() const;

};

tcphdr *TCPPacket::getTcpHeader() const {
    auto tcph = (tcphdr *) buffer + sizeof(iphdr);
    return tcph;
}


void TCPPacket::setEnds(sockaddr_in &src, sockaddr_in &dest) {
    auto iph = getIpHeader();
    auto tcph = getTcpHeader();

    iph->saddr = src.sin_addr.s_addr;
    iph->daddr = dest.sin_addr.s_addr;

    tcph->source = src.sin_port;
    tcph->dest = dest.sin_port;
}

void TCPPacket::swapEnds() {
    auto iph = getIpHeader();
    auto tcph = getTcpHeader();

    auto src = iph->saddr;
    auto srcPort = tcph->source;

    iph->saddr = iph->daddr;
    iph->daddr = src;

    tcph->source = tcph->dest;
    tcph->dest = srcPort;

}

void TCPPacket::setSource(sockaddr_in &src) {
    auto iph = getIpHeader();
    auto tcph = getTcpHeader();

    iph->saddr = src.sin_addr.s_addr;
    tcph->source = src.sin_port;
}

void TCPPacket::setDestination(sockaddr_in &dest) {
    auto iph = getIpHeader();
    auto tcph = getTcpHeader();

    iph->daddr = dest.sin_addr.s_addr;
    tcph->dest = dest.sin_port;
}


void TCPPacket::setSequenceNumber(unsigned int seq) {
    auto tcph = getTcpHeader();
    tcph->seq = htons(seq);
}

void TCPPacket::setAcknowledgmentNumber(unsigned int ack) {
    auto tcph = getTcpHeader();
    tcph->ack_seq = htons(ack);
    if (ack) tcph->ack = true;
}

void TCPPacket::setUrgentPointer(unsigned short urg) {
    auto tcph = getTcpHeader();
    tcph->urg_ptr = htons(urg);
    if (urg) tcph->urg = true;
}


void TCPPacket::setFinFlag(bool isFin) {
    if (isFin) getTcpHeader()->fin = 1;
    else getTcpHeader()->fin = 0;
}

void TCPPacket::setSynFlag(bool isSyn) {
    if (isSyn) getTcpHeader()->syn = 1;
    else getTcpHeader()->syn = 0;
}

void TCPPacket::setResetFlag(bool isRst) {
    if (isRst) getTcpHeader()->rst = 1;
    else getTcpHeader()->rst = 0;
}

void TCPPacket::setPushFlag(bool isPush) {
    if (isPush) getTcpHeader()->psh = 1;
    else getTcpHeader()->psh = 0;
}

void TCPPacket::setWindowSize(unsigned short window, unsigned char shift) {
    setWindowScale(shift);
    auto tcph = getTcpHeader();
    tcph->window = htons(window);
}

void TCPPacket::setWindowSize(unsigned short window) {
    auto tcph = getTcpHeader();
    tcph->window = htons(window);
}

void TCPPacket::setWindowScale(unsigned char shift) {
    if (shift > 14)throw invalid_argument("Invalid window scale shift: " + to_string(shift));
    setOption(3, 3, &shift);
}

void TCPPacket::setMSS(unsigned short size) {
    size = htons(size);
    setOption(2, 4, (unsigned char *) &size);
}

sockaddr_in TCPPacket::getSource() {//warn: skipped memset to zero
    auto tcph = getTcpHeader();
    auto iph = getIpHeader();
    sockaddr_in addr{AF_INET, tcph->source, iph->saddr};
    return addr;
}

inline bool TCPPacket::isFrom(sockaddr_in &src) {
    auto tcph = getTcpHeader();
    auto iph = getIpHeader();
    return src.sin_port == tcph->source && src.sin_addr.s_addr == iph->saddr;
}

bool TCPPacket::isTo(sockaddr_in &dest) {
    auto tcph = getTcpHeader();
    auto iph = getIpHeader();
    return dest.sin_port == tcph->dest && dest.sin_addr.s_addr == iph->daddr;
}

sockaddr_in TCPPacket::getDestination() {//warn: skipped memset to zero
    auto tcph = getTcpHeader();
    auto iph = getIpHeader();
    sockaddr_in addr{AF_INET, tcph->dest, iph->daddr};
    return addr;
}


unsigned int TCPPacket::getSequenceNumber() {
    auto tcph = getTcpHeader();
    return ntohl(tcph->seq);
}

unsigned int TCPPacket::getAcknowledgmentNumber() {
    if (!isAck())return 0;
    else {
        auto tcph = getTcpHeader();
        return ntohl(tcph->ack_seq);
    }
}

unsigned short TCPPacket::getUrgentPointer() {
    if (!isUrg()) return 0;
    else {
        auto tcph = getTcpHeader();
        return ntohl(tcph->ack_seq);
    }
}

bool TCPPacket::isSyn() const {
    auto tcph = getTcpHeader();
    return tcph->syn;
}

bool TCPPacket::isAck() const {
    auto tcph = getTcpHeader();
    return tcph->ack;
}

bool TCPPacket::isUrg() const {
    auto tcph = getTcpHeader();
    return tcph->urg;
}

bool TCPPacket::isFin() const {
    auto tcph = getTcpHeader();
    return tcph->fin;
}

bool TCPPacket::isReset() const {
    auto tcph = getTcpHeader();
    return tcph->rst;
}

bool TCPPacket::isPush() const {
    auto tcph = getTcpHeader();
    return tcph->psh;
}

unsigned short TCPPacket::getWindowSize() {
    auto tcph = getTcpHeader();
    return ntohs(tcph->window);
}

unsigned char TCPPacket::getWindowShift() {
    unsigned char data;
    unsigned char len = getOption(3, &data, 1);
    if (len != 3)throw invalid_argument("Invalid window scale length encountered: " + to_string(len));
    return data;
}

unsigned short TCPPacket::getMSS() {
    unsigned short result;
    unsigned int len = getOption(2, (unsigned char *) &result, 2);
    if (len != 2) throw invalid_argument("Packet with invalid mss len: " + to_string(len));
    return ntohs(result);
}

unsigned int TCPPacket::appendData(unsigned char *data, unsigned int len) {
    unsigned int total = min(len, maxSize - length);
    memcpy(buffer + length, data, total);
    return total;
}

void TCPPacket::clearData() {
    length = OPTIONS_INDEX + optionsLength;
}


unsigned int TCPPacket::getDataLength() const {
    return length - OPTIONS_INDEX - optionsLength;
}

unsigned int TCPPacket::copyDataTo(unsigned char *buff, unsigned int len) {
    unsigned int total = min(len, getDataLength());
    memcpy(buff, buffer + OPTIONS_INDEX + optionsLength, total);
    return total;
}

void TCPPacket::makeSyn(unsigned int seq, unsigned int ack) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(ack);
    clearData();

    auto tcph = getTcpHeader();
    tcph->syn = true;
    tcph->rst = false;
    tcph->fin = false;
}

void TCPPacket::makeFin(unsigned int seq, unsigned int ack) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(ack);

    auto tcph = getTcpHeader();
    tcph->syn = false;
    tcph->rst = false;
    tcph->fin = true;
}

void TCPPacket::makeResetAck(unsigned int ack) {
    setAcknowledgmentNumber(ack);
    setSequenceNumber(0);
    clearData();

    auto tcph = getTcpHeader();
    tcph->syn = false;
    tcph->rst = true;
    tcph->fin = false;
}

void TCPPacket::makeResetSeq(unsigned int seq) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(0);
    clearData();

    auto tcph = getTcpHeader();
    tcph->syn = false;
    tcph->rst = true;
    tcph->fin = false;
}

void TCPPacket::makeNormal(unsigned int seqNo, unsigned int ackSeq) {
    setSequenceNumber(seqNo);
    setAcknowledgmentNumber(ackSeq);

    auto tcph = getTcpHeader();
    tcph->syn = false;
    tcph->rst = false;
    tcph->fin = false;
};

inline unsigned int TCPPacket::getSegmentLength() const {
    unsigned int result = getDataLength();
    if (isSyn())result++;
    if (isFin())result++;
    return result;
}

struct pseudo_header {
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t tcp_length;
};


#endif //TUNSERVER_PACKET_H

#pragma clang diagnostic pop