//
// Created by yoni_ash on 4/22/23.
//

#ifndef TUNSERVER_COMPRESSED_IPPACKET_H
#define TUNSERVER_COMPRESSED_IPPACKET_H


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma once

#include <memory>
#include <utility>

#include "../Include.h"
#include "../helpers/buffer/SmartBuffer.h"

#define MIN_SS 576

inline unsigned int toNetworkByteOrder(unsigned int i) {
    return htonl(i);
}

inline unsigned short toNetworkByteOrder(unsigned short i) {
    return htons(i);
}

inline unsigned int toHostByteOrder(unsigned int i) {
    return ntohl(i);
}

inline unsigned short toHostByteOrder(unsigned short i) {
    return ntohs(i);
}


class CompressedIPPacket {
private:
    struct CompressedIPH {
        unsigned char protocol{};
        unsigned int sourceIP{};
        unsigned int destinationIP{};
    };

    struct Data {
        char *mBuffer{};
        unsigned short mBufferSize{};
        shared_ptr<void> handle{};
        SmartBuffer<char> payload{0};
        unsigned short payloadOffset{mBufferSize};
    };

public:

    CompressedIPPacket() : CompressedIPPacket(0) {};

    inline explicit CompressedIPPacket(unsigned short extra);

    inline explicit CompressedIPPacket(void *buf, unsigned short size, shared_ptr<void> handle = nullptr,
                                       SmartBuffer<char> payload = SmartBuffer<char>{0});

    inline CompressedIPPacket(const CompressedIPPacket &old) = default;

    inline CompressedIPPacket(CompressedIPPacket &&old) noexcept = default;

    inline CompressedIPPacket &operator=(const CompressedIPPacket &other) = default;

    inline CompressedIPPacket &operator=(CompressedIPPacket &&other) noexcept = default;

    inline virtual void setPayload(SmartBuffer<char> &buf, unsigned short offset);

    [[nodiscard]]inline SmartBuffer<char> &getPayload() const;

    [[nodiscard]] inline char *getBuffer() const;

    [[nodiscard]] inline unsigned short getBufferSize() const;

    virtual bool isValid();

    virtual void validate();

    [[nodiscard]] inline unsigned int getSourceIP() const;

    [[nodiscard]] inline unsigned int getDestinationIP() const;

    inline void setSourceIP(unsigned int ip);

    inline void setDestinationIP(unsigned int ip);

    [[nodiscard]] inline unsigned short getLength() const;

    [[nodiscard]] inline unsigned short getProtocol() const;

    inline void setProtocol(unsigned short proto);

    inline unsigned short writeTo(void *buf, unsigned short len, unsigned short off = 0);

    template<typename Object>
    inline unsigned short writeToObject(Object &obj);

    inline unsigned short readFrom(void *buf, unsigned short len, unsigned short off = 0);

    template<typename Object>
    inline unsigned short readFromObject(Object &obj);

    static inline unsigned short getIPHeaderSize();

    friend class DisposableTunnel;

protected:
    shared_ptr<Data> data{};

    [[nodiscard]] inline CompressedIPH &getIPHeader() const;
};

CompressedIPPacket::CompressedIPPacket(unsigned short extra) {
    data = std::make_shared<Data>(
            Data{new char[getIPHeaderSize() + extra]{}, static_cast<unsigned short>(getIPHeaderSize() + extra)});
}


unsigned int CompressedIPPacket::getSourceIP() const {
    return toHostByteOrder(getIPHeader().sourceIP);
}

unsigned int CompressedIPPacket::getDestinationIP() const {
    return toHostByteOrder(getIPHeader().destinationIP);
}

void CompressedIPPacket::setSourceIP(unsigned int ip) {
    getIPHeader().sourceIP = toNetworkByteOrder(ip);
}

void CompressedIPPacket::setDestinationIP(unsigned int ip) {
    getIPHeader().destinationIP = toNetworkByteOrder(ip);
}

unsigned short CompressedIPPacket::getBufferSize() const {
    return data->mBufferSize;
}

unsigned short CompressedIPPacket::getProtocol() const {
    return getIPHeader().protocol;
}


unsigned short CompressedIPPacket::getIPHeaderSize() {
    return sizeof(CompressedIPH);
}

CompressedIPPacket::CompressedIPH &CompressedIPPacket::getIPHeader() const {
    return *(CompressedIPH *) data->mBuffer;
}

unsigned short CompressedIPPacket::getLength() const {
    return data->payloadOffset + data->payload.getLimit();
}

void CompressedIPPacket::setPayload(SmartBuffer<char> &buf, unsigned short offset) {
    if (offset > data->mBufferSize || offset < getIPHeaderSize())throw out_of_range("Invalid remote buffer offset");
    data->payloadOffset = offset;
    data->payload = buf;
}

SmartBuffer<char> &CompressedIPPacket::getPayload() const {
    return data->payload;
}

char *CompressedIPPacket::getBuffer() const {
    return data->mBuffer;
}

bool CompressedIPPacket::isValid() { return true; }

void CompressedIPPacket::validate() {}

void CompressedIPPacket::setProtocol(unsigned short proto) {
    getIPHeader().protocol = proto;
}

unsigned short CompressedIPPacket::writeTo(void *buf, unsigned short len, unsigned short off) {
    auto remaining = len;

    if (off < data->payloadOffset) {
        auto amt = min((unsigned short) (data->payloadOffset - off), remaining);
        memcpy(buf, data->mBuffer + off, amt);
        buf = (char *) buf + amt;
        remaining -= amt;
    }

    if (remaining) {
        auto amt = min(remaining, (unsigned short) data->payload.available());
        data->payload.get((char *) buf, amt, off); //IMP: can be compiled without a cast to char *
        remaining -= amt;
    }

    return len - remaining;
}

unsigned short CompressedIPPacket::readFrom(void *buf, unsigned short len, unsigned short off) {
    auto remaining = len;

    if (off < data->payloadOffset) {
        auto amt = min((unsigned short) (data->payloadOffset - off), (unsigned short) remaining);
        memcpy(data->mBuffer + off, buf, amt);
        remaining -= amt;
        buf = (char *) buf + amt;
    }

    if (remaining) {
        data->payload.rewind();
        auto amt = min(remaining, (unsigned short) data->payload.available());
        data->payload.put((char *) buf, amt);
        remaining -= amt;
    }

    return len - remaining;
}

CompressedIPPacket::CompressedIPPacket(void *buf, unsigned short size, shared_ptr<void> handle,
                                       SmartBuffer<char> payload) : data(
        new Data{(char *) buf, size, std::move(handle), payload}) {
    if (size < getIPHeaderSize())throw length_error("Invalid ip packet size");
}

template<typename Object>
unsigned short CompressedIPPacket::writeToObject(Object &obj) {
    writeTo(&obj, sizeof obj);
}

template<typename Object>
unsigned short CompressedIPPacket::readFromObject(Object &obj) {
    readFrom(&obj, sizeof obj);
}


#endif //TUNSERVER_COMPRESSED_IPPACKET_H

#pragma clang diagnostic pop