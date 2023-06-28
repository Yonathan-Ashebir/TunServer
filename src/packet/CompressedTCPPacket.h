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
#include "CompressedIPPacket.h"

class CompressedTCPPacket : public CompressedIPPacket {
private:
    struct CompressedTCPH {
        unsigned short sourcePort{};
        unsigned short destinationPort{};
        unsigned int sequenceNumber{};
        unsigned int acknowledgementNumber{};
#ifdef __LITTLE_ENDIAN
        bool fin: 1{};
        bool syn: 1{};
        bool rst: 1{};
        bool psh: 1{};
        bool ack: 1{};
        bool urg: 1{};
        bool ece: 1{};
        bool cwr: 1{};
#else
        bool cwr: 1{};
        bool ece: 1{};
        bool urg: 1{};
        bool ack: 1{};
        bool psh: 1{};
        bool rst: 1{};
        bool syn: 1{};
        bool fin: 1{};
#endif
        unsigned short window{};
        unsigned char windowShift{};
    };
public:

    explicit CompressedTCPPacket(unsigned int size);

    inline void setPorts(unsigned short src, unsigned short dest);

    inline void setSourcePort(unsigned short port);

    inline void setDestinationPort(unsigned short port);

    inline void setSequenceNumber(unsigned int seq);

    inline void setAcknowledgmentNumber(unsigned int ack);

    inline void setSynFlag(bool isSyn);

    inline void setAckFlag(bool isAck);;

    inline void setFinFlag(bool isFin);

    inline void setResetFlag(bool isRst);

    inline void setPushFlag(bool isPush);

    inline void setWindowSize(unsigned short window);

    inline void setWindowScale(unsigned char shift);

    inline void setWindowSize(unsigned short window, unsigned char shift);

    inline void setMSS(unsigned short size);

    [[nodiscard]]  inline unsigned short getSourcePort() const;

    [[nodiscard]] inline unsigned short getDestination() const;;

    [[nodiscard]] inline unsigned int getSequenceNumber() const;

    [[nodiscard]] inline unsigned int getAcknowledgmentNumber() const;

    [[nodiscard]] inline bool isSyn() const;

    [[nodiscard]] inline bool isFin() const;

    [[nodiscard]] inline bool isReset() const;

    [[nodiscard]] inline bool isPush() const;

    [[nodiscard]] inline bool isAck() const;

    [[nodiscard]] inline unsigned short getWindowSize() const;

    [[nodiscard]] inline unsigned char getWindowShift() const;

    [[nodiscard]] inline unsigned short getMSS() const;

    [[nodiscard]] inline unsigned short getPayloadSize() const;

    inline unsigned short copyPayloadTo(void *buff, unsigned short len) const;

    inline void makeSyn(unsigned int seq, unsigned int ack);

    inline void makeResetSeq(unsigned int seq);

    inline void makeResetAck(unsigned int ack);

    inline void makeFin(unsigned int seq, unsigned int ack);

    inline void makeNormal(unsigned int seqNo, unsigned int ackSeq);

    inline void swapEnds();

    [[nodiscard]] inline unsigned int getSegmentLength() const;


protected:

private:

    [[nodiscard]] inline CompressedTCPH &getTCPHeader() const;

    [[nodiscard]] inline unsigned short getTCPHeaderSize() const;

};

CompressedTCPPacket::CompressedTCPH &CompressedTCPPacket::getTCPHeader() const {
    if (data->remoteOffset > getIPHeaderSize()) {
        if (data->remoteOffset < getIPHeaderSize() + getTCPHeaderSize())
            throw logic_error("Invalid inner buffer capacity for tcp");
        return *(CompressedTCPH *) data->mBuffer;
    } else {
        if (getIPHeaderSize() - data->remoteOffset + getTCPHeaderSize() > data->remoteBufferSize)
            throw logic_error("Invalid remote buffer capacity for tcp");
        return *(CompressedTCPH *) (data->mBuffer + getIPHeaderSize());
    }
}


void CompressedTCPPacket::swapEnds() {
    auto iph = getIPHeader();
    auto tcph = getTCPHeader();

    auto src = iph.sourceIP;
    auto srcPort = tcph.sourcePort;

    iph.sourceIP = iph.destinationIP;
    iph.destinationIP = src;

    tcph.sourcePort = tcph.destinationPort;
    tcph.destinationPort = srcPort;

}


void CompressedTCPPacket::setSequenceNumber(unsigned int seq) {
    auto tcph = getTCPHeader();
    tcph.sequenceNumber = toNetworkByteOrder(seq);
}

void CompressedTCPPacket::setAcknowledgmentNumber(unsigned int ack) {
    auto tcph = getTCPHeader();
    tcph.acknowledgementNumber = toNetworkByteOrder(ack);
    if (ack) tcph.ack = true;
}


void CompressedTCPPacket::setFinFlag(bool isFin) {
    getTCPHeader().fin = isFin;

}

void CompressedTCPPacket::setSynFlag(bool isSyn) {
    getTCPHeader().syn = isSyn;
}

void CompressedTCPPacket::setResetFlag(bool isRst) {
    getTCPHeader().rst = isRst;
}

void CompressedTCPPacket::setPushFlag(bool isPush) {
    getTCPHeader().psh = isPush;
}

void CompressedTCPPacket::setWindowSize(unsigned short window, unsigned char shift) {
    setWindowScale(shift);
    getTCPHeader().window = htons(window);
}

void CompressedTCPPacket::setWindowSize(unsigned short window) {
    auto tcph = getTCPHeader();
    tcph.window = htons(window);
}

void CompressedTCPPacket::setWindowScale(unsigned char shift) {
    if (shift > 14)throw out_of_range("TCP window shift can not be over 14");
    getTCPHeader().windowShift = shift;
}

void CompressedTCPPacket::setMSS(unsigned short size) {
//todo
}


unsigned int CompressedTCPPacket::getSequenceNumber() const {
    return toHostByteOrder(getTCPHeader().sequenceNumber);
}

unsigned int CompressedTCPPacket::getAcknowledgmentNumber() const {
    if (!isAck())return 0;
    return toHostByteOrder(getTCPHeader().acknowledgementNumber);
}

bool CompressedTCPPacket::isSyn() const {
    auto tcph = getTCPHeader();
    return tcph.syn;
}

bool CompressedTCPPacket::isAck() const {
    auto tcph = getTCPHeader();
    return tcph.ack;
}


bool CompressedTCPPacket::isFin() const {
    auto tcph = getTCPHeader();
    return tcph.fin;
}

bool CompressedTCPPacket::isReset() const {
    auto tcph = getTCPHeader();
    return tcph.rst;
}

bool CompressedTCPPacket::isPush() const {
    auto tcph = getTCPHeader();
    return tcph.psh;
}

unsigned short CompressedTCPPacket::getWindowSize() const {
    auto tcph = getTCPHeader();
    return ntohs(tcph.window);
}

unsigned char CompressedTCPPacket::getWindowShift() const {
    return getTCPHeader().windowShift;
}

unsigned short CompressedTCPPacket::getMSS() const {
    return 2 * MIN_MSS;
}

unsigned short CompressedTCPPacket::getPayloadSize() const {
    return getLength() - getTCPHeaderSize() - getIPHeaderSize();
}

unsigned short CompressedTCPPacket::copyPayloadTo(void *buff, unsigned short len) const {
    unsigned short remaining = len;
    if (data->remoteOffset > getIPHeaderSize() + getTCPHeaderSize()) {
        unsigned short amt = min(remaining,
                                 (unsigned short) (data->remoteOffset - getIPHeaderSize() - getTCPHeaderSize()));
        memcpy(buff, data->mBuffer + getIPHeaderSize() + getTCPHeaderSize(), amt);
        remaining -= amt;
    }
    if (remaining && data->remoteBuffer) {
        unsigned short amt = min(remaining,
                                 (unsigned short) (data->remoteBufferSize - data->remoteOffset + getIPHeaderSize() +
                                                   getTCPHeaderSize()));
        memcpy(buff, data->mBuffer + getIPHeaderSize() + getTCPHeaderSize(), amt);
        remaining -= amt;
    }
    return len - remaining;
}

void CompressedTCPPacket::makeSyn(unsigned int seq, unsigned int ack) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(ack);

    auto tcph = getTCPHeader();
    tcph.syn = true;
    tcph.rst = false;
    tcph.fin = false;
}

void CompressedTCPPacket::makeFin(unsigned int seq, unsigned int ack) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(ack);

    auto tcph = getTCPHeader();
    tcph.syn = false;
    tcph.rst = false;
    tcph.fin = true;
}

void CompressedTCPPacket::makeResetAck(unsigned int ack) {
    setAcknowledgmentNumber(ack);
    setSequenceNumber(0);

    auto tcph = getTCPHeader();
    tcph.syn = false;
    tcph.rst = true;
    tcph.fin = false;
}

void CompressedTCPPacket::makeResetSeq(unsigned int seq) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(0);

    auto tcph = getTCPHeader();
    tcph.syn = false;
    tcph.rst = true;
    tcph.fin = false;
}

void CompressedTCPPacket::makeNormal(unsigned int seqNo, unsigned int ackSeq) {
    setSequenceNumber(seqNo);
    setAcknowledgmentNumber(ackSeq);
    setSynFlag(false);
    setPushFlag(false);
    setFinFlag(false);
    setResetFlag(false);
    setProtocol(IPPROTO_TCP);
}

inline unsigned int CompressedTCPPacket::getSegmentLength() const {
    unsigned int result = getPayloadSize();
    if (isSyn())result++;
    if (isFin())result++;
    return result;
}

unsigned short CompressedTCPPacket::getTCPHeaderSize() const {
    return sizeof(CompressedTCPH);
}

void CompressedTCPPacket::setSourcePort(unsigned short port) {
    getTCPHeader().sourcePort = port;
}

void CompressedTCPPacket::setDestinationPort(unsigned short port) {
    getTCPHeader().destinationPort = port;
}

void CompressedTCPPacket::setPorts(unsigned short src, unsigned short dest) {
    setSourcePort(src);
    setDestinationPort(dest);
}

void CompressedTCPPacket::setAckFlag(bool isAck) {
    getTCPHeader().ack = isAck;
}

unsigned short CompressedTCPPacket::getSourcePort() const {
    return getTCPHeader().sourcePort;
}

unsigned short CompressedTCPPacket::getDestination() const {
    return getTCPHeader().destinationPort;
}

CompressedTCPPacket::CompressedTCPPacket(unsigned int size) : CompressedIPPacket(size) {
    if (size != 0) {
        if (size > getIPHeaderSize() && size < getIPHeaderSize() + getTCPHeaderSize())
            throw invalid_argument("Invalid packet capacity for tcp");
        getIPHeader().protocol = IPPROTO_TCP;
    }
}


#endif //TUNSERVER_PACKET_H

#pragma clang diagnostic pop