//
// Created by yoni_ash on 4/21/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#ifndef TUNSERVER_COMPRESSED_TCP_PACKET_H
#define TUNSERVER_COMPRESSED_TCP_PACKET_H


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
#if __BYTE_ORDER == __LITTLE_ENDIAN
        bool fin: 1{};
        bool syn: 1{};
        bool rst: 1{};
        bool psh: 1{};
        bool ack: 1{};
        bool urg: 1{};
        bool ece: 1{};
        bool cwr: 1{};
#elif __BYTE_ORDER == __BIG_ENDIAN
        bool cwr: 1{};
        bool ece: 1{};
        bool urg: 1{};
        bool ack: 1{};
        bool psh: 1{};
        bool rst: 1{};
        bool syn: 1{};
        bool fin: 1{};
#else
# error        "Please fix <bits/endian.h>"
#endif
        unsigned short window{};
        unsigned char windowShift{};
        unsigned char mss{};
    };
public:

    inline explicit CompressedTCPPacket();

    inline  explicit CompressedTCPPacket(unsigned int extra);

    inline void swapEnds();

    inline void setPorts(unsigned short src, unsigned short dest);

    inline void setSourcePort(unsigned short port);

    inline void setDestinationPort(unsigned short port);

    inline void setSourceAddress(sockaddr_storage &addr);

    inline void setDestinationAddress(sockaddr_storage &addr);

    inline sockaddr_storage getSourceAddress();

    inline sockaddr_storage getDestinationAddress();

    inline void setSequenceNumber(unsigned int seq);

    inline void setAcknowledgmentNumber(unsigned int ack);

    inline void setSynFlag(bool isSyn);

    inline void setAckFlag(bool isAck);;

    inline void setFinFlag(bool isFin);

    inline void setResetFlag(bool isRst);

    inline void setPushFlag(bool isPush);

    inline void setWindow(unsigned int total);

    inline void setWindowSize(unsigned short window, unsigned char shift);

    inline void setMSS(unsigned short size);

    [[nodiscard]]  inline unsigned short getSourcePort() const;

    [[nodiscard]] inline unsigned short getDestinationPort() const;;

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

    [[nodiscard]] inline unsigned short getTCPPayloadSize() const;

    inline unsigned short copyTCPPayloadTo(void *buf, unsigned short len) const;

    inline void setTCPPayload(SmartBuffer<char> &buf);

    inline void setTCPPayload(SmartBuffer<char> &&buf);

    inline void makeSyn(unsigned int seq, unsigned int ack);

    inline void makeResetSeq(unsigned int seq);

    inline void makeResetAck(unsigned int ack);

    inline void makeFin(unsigned int seq, unsigned int ack);

    inline void makeNormal(unsigned int seqNo, unsigned int ackSeq);

    [[nodiscard]] inline unsigned int getSegmentLength() const;

    [[nodiscard]]  static inline int getTCPPayloadOffset();


protected:

private:

    [[nodiscard]] inline CompressedTCPH &getTCPHeader() const;

    [[nodiscard]] static inline unsigned short getTCPHeaderSize();

};

CompressedTCPPacket::CompressedTCPH &CompressedTCPPacket::getTCPHeader() const {
    if (data->payloadOffset < getTCPPayloadOffset())
        throw logic_error("Invalid available inner buffer size for tcp");
    return *(CompressedTCPH *) (data->mBuffer + getIPHeaderSize());
}


void CompressedTCPPacket::swapEnds() {
    auto &iph = getIPHeader();
    auto &tcph = getTCPHeader();

    auto src = iph.sourceIP;
    auto srcPort = tcph.sourcePort;

    iph.sourceIP = iph.destinationIP;
    iph.destinationIP = src;

    tcph.sourcePort = tcph.destinationPort;
    tcph.destinationPort = srcPort;

}


void CompressedTCPPacket::setSequenceNumber(unsigned int seq) {
    auto &tcph = getTCPHeader();
    tcph.sequenceNumber = toNetworkByteOrder(seq);
}

void CompressedTCPPacket::setAcknowledgmentNumber(unsigned int ack) {
    auto &tcph = getTCPHeader();
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
    auto &tcph = getTCPHeader();
    tcph.window = toNetworkByteOrder(window);
    tcph.windowShift = min(shift, (unsigned char) 14);
}

void CompressedTCPPacket::setWindow(unsigned int total) {
    auto &tcph = getTCPHeader();
    tcph.window = toNetworkByteOrder(
            static_cast<unsigned short>(min(total, static_cast<unsigned int>(numeric_limits<unsigned short>::max()))));
    tcph.windowShift = static_cast<char>(min(log2(total / numeric_limits<unsigned short>::max()),
                                             static_cast<double>(14)));
}

void CompressedTCPPacket::setMSS(unsigned short size) {
    getTCPHeader().mss = size >> 8;
}


unsigned int CompressedTCPPacket::getSequenceNumber() const {
    return toHostByteOrder(getTCPHeader().sequenceNumber);
}

unsigned int CompressedTCPPacket::getAcknowledgmentNumber() const {
    if (!isAck())return 0;
    return toHostByteOrder(getTCPHeader().acknowledgementNumber);
}

bool CompressedTCPPacket::isSyn() const {
    auto &tcph = getTCPHeader();
    return tcph.syn;
}

bool CompressedTCPPacket::isAck() const {
    auto &tcph = getTCPHeader();
    return tcph.ack;
}


bool CompressedTCPPacket::isFin() const {
    auto &tcph = getTCPHeader();
    return tcph.fin;
}

bool CompressedTCPPacket::isReset() const {
    auto &tcph = getTCPHeader();
    return tcph.rst;
}

bool CompressedTCPPacket::isPush() const {
    auto &tcph = getTCPHeader();
    return tcph.psh;
}

unsigned short CompressedTCPPacket::getWindowSize() const {
    auto &tcph = getTCPHeader();
    return toHostByteOrder(tcph.window);
}

unsigned char CompressedTCPPacket::getWindowShift() const {
    return getTCPHeader().windowShift;
}

unsigned short CompressedTCPPacket::getMSS() const {
    return getTCPHeader().mss << 8;
}

unsigned short CompressedTCPPacket::getTCPPayloadSize() const {
    return getLength() - getTCPHeaderSize() - getIPHeaderSize();
}

unsigned short CompressedTCPPacket::copyTCPPayloadTo(void *buf, unsigned short len) const {
    if (data->payloadOffset < getTCPPayloadOffset())
        throw logic_error("Invalid available inner buffer size for tcp");
    unsigned short remaining = len;
    unsigned short amt = min(remaining,
                             (unsigned short) (data->payloadOffset - getIPHeaderSize() - getTCPHeaderSize()));
    memcpy(buf, data->mBuffer, amt);
    remaining -= amt;
    buf = (char *) buf + amt;

    if (remaining) {
        data->payload.rewind();
        amt = min(remaining, (unsigned short) data->payload.available());
        data->payload.get((char *) buf, amt);
        remaining -= amt;
    }

    return len - remaining;
}

void CompressedTCPPacket::makeSyn(unsigned int seq, unsigned int ack) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(ack);

    auto &tcph = getTCPHeader();
    tcph.syn = true;
    tcph.rst = false;
    tcph.fin = false;
}

void CompressedTCPPacket::makeFin(unsigned int seq, unsigned int ack) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(ack);

    auto &tcph = getTCPHeader();
    tcph.syn = false;
    tcph.rst = false;
    tcph.fin = true;
}

void CompressedTCPPacket::makeResetAck(unsigned int ack) {
    setAcknowledgmentNumber(ack);
    setSequenceNumber(0);

    auto &tcph = getTCPHeader();
    tcph.syn = false;
    tcph.rst = true;
    tcph.fin = false;
}

void CompressedTCPPacket::makeResetSeq(unsigned int seq) {
    setSequenceNumber(seq);
    setAcknowledgmentNumber(0);

    auto &tcph = getTCPHeader();
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
    return getTCPPayloadSize() + isFin() + isSyn();
}

unsigned short CompressedTCPPacket::getTCPHeaderSize() {
    return sizeof(CompressedTCPH);
}

void CompressedTCPPacket::setSourcePort(unsigned short port) {
    getTCPHeader().sourcePort = toNetworkByteOrder(port);
}

void CompressedTCPPacket::setDestinationPort(unsigned short port) {
    getTCPHeader().destinationPort = toNetworkByteOrder(port);
}

void CompressedTCPPacket::setPorts(unsigned short src, unsigned short dest) {
    setSourcePort(src);
    setDestinationPort(dest);
}

void CompressedTCPPacket::setAckFlag(bool isAck) {
    getTCPHeader().ack = isAck;
}

unsigned short CompressedTCPPacket::getSourcePort() const {
    return toHostByteOrder(getTCPHeader().sourcePort);
}

unsigned short CompressedTCPPacket::getDestinationPort() const {
    return toHostByteOrder(getTCPHeader().destinationPort);
}

CompressedTCPPacket::CompressedTCPPacket(unsigned int extra) : CompressedIPPacket(extra + getTCPHeaderSize()) {}

CompressedTCPPacket::CompressedTCPPacket() : CompressedTCPPacket(0) {

}

void CompressedTCPPacket::setSourceAddress(sockaddr_storage &addr) {
    if (addr.ss_family == AF_INET) {
        auto inAddr = reinterpret_cast<sockaddr_in * >(&addr);
        getIPHeader().sourceIP = inAddr->sin_addr.s_addr;
        getTCPHeader().sourcePort = inAddr->sin_port;
    } else {
        //todo: implement ipv6
        throw logic_error("Family isn't supported");
    }
}

void CompressedTCPPacket::setDestinationAddress(sockaddr_storage &addr) {
    if (addr.ss_family == AF_INET) {
        auto inAddr = reinterpret_cast<sockaddr_in * >(&addr);
        getIPHeader().destinationIP = inAddr->sin_addr.s_addr;
        getTCPHeader().destinationPort = inAddr->sin_port;
    } else {
        //todo: implement ipv6
        throw logic_error("Family isn't supported");
    }
}

sockaddr_storage CompressedTCPPacket::getSourceAddress() {
    //todo: implement ipv6
    sockaddr_storage result{AF_INET};
    auto inAddr = reinterpret_cast<sockaddr_in * >(&result);
    inAddr->sin_addr.s_addr = getIPHeader().sourceIP;
    inAddr->sin_port = getTCPHeader().sourcePort;
    return result;
}

sockaddr_storage CompressedTCPPacket::getDestinationAddress() {
    //todo: implement ipv6
    sockaddr_storage result{AF_INET};
    auto inAddr = reinterpret_cast<sockaddr_in * >(&result);
    inAddr->sin_addr.s_addr = getIPHeader().destinationIP;
    inAddr->sin_port = getTCPHeader().destinationPort;
    return result;
}

int CompressedTCPPacket::getTCPPayloadOffset() {
    return getIPHeaderSize() + getTCPHeaderSize();
}

void CompressedTCPPacket::setTCPPayload(SmartBuffer<char> &buf) {
    setPayload(buf, getTCPPayloadOffset());
}

void CompressedTCPPacket::setTCPPayload(SmartBuffer<char> &&buf) {
    setTCPPayload(buf);
}


#endif //TUNSERVER_COMPRESSED_TCP_PACKET_H

#pragma clang diagnostic pop