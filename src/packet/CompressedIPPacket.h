//
// Created by yoni_ash on 4/22/23.
//

#ifndef TUNSERVER_COMPRESSED_IPPACKET_H
#define TUNSERVER_COMPRESSED_IPPACKET_H


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma once

#include "../Include.h"

#define MIN_MSS 576

inline unsigned int toNetworkByteOrder(unsigned int i) {
    return htonl(i);
}

inline unsigned short toNetworkByteOrder(unsigned short i) {
    return ntohs(i);
}

inline unsigned int toHostByteOrder(unsigned int i) {
    return htons(i);
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

        char *remoteBuffer{};
        unsigned short remoteBufferSize{};
        unsigned short remoteOffset{};

        unsigned int count{1};
    };

public:

    CompressedIPPacket() : CompressedIPPacket(0) {};

    inline ~CompressedIPPacket();

    inline CompressedIPPacket(const CompressedIPPacket &old);

    inline CompressedIPPacket(CompressedIPPacket &&old) noexcept;;

    inline CompressedIPPacket &operator=(const CompressedIPPacket &other);

    inline CompressedIPPacket &operator=(CompressedIPPacket &&other) noexcept;

    inline virtual void setRemoteBuffer(char *buf, unsigned short len, unsigned short offset);

    [[nodiscard]]inline char *getRemoteBuffer() const;

    [[nodiscard]] inline unsigned short getRemoteBufferSize() const;

    [[nodiscard]] inline char *getBuffer() const;

    virtual bool isValid();

    virtual void validate();

    [[nodiscard]] inline unsigned int getSourceIP() const;

    [[nodiscard]] inline unsigned int getDestinationIP() const;

    inline void setSourceIP(unsigned int ip);

    inline void setDestinationIP(unsigned int ip);

    [[nodiscard]] inline unsigned short getBufferSize() const;

    [[nodiscard]] inline unsigned short getLength() const;

    [[nodiscard]] inline unsigned short getProtocol() const;

    inline void setProtocol(unsigned short proto);

    inline unsigned short writeTo(void *buf, unsigned short size);

    template<typename Object>
    inline unsigned short writeToObject(Object &obj) {
        writeTo(&obj, sizeof obj);
    }

    inline unsigned short readFrom(void *buf, unsigned short size);

    template<typename Object>
    inline unsigned short readFromObject(Object &obj);

    static inline unsigned short getIPHeaderSize();

    friend class DisposableTunnel;

protected:
    Data *data;

    inline explicit CompressedIPPacket(unsigned short size);

    [[nodiscard]] inline CompressedIPH &getIPHeader() const;
};

CompressedIPPacket::CompressedIPPacket(unsigned short size) {
    if (size && size < getIPHeaderSize())throw length_error("Invalid ip packet capacity");
    data = new Data{size ? new char[getIPHeaderSize() + size] : nullptr, size, nullptr, 0, size};
}


CompressedIPPacket::~CompressedIPPacket() {
    if (data && !--data->count) {
        delete data->mBuffer;
        delete data;
    }
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
    if (data->remoteOffset) return *(CompressedIPH *) data->mBuffer;
    else return *(CompressedIPH *) data->remoteBuffer;
}

unsigned short CompressedIPPacket::getLength() const {
    return data->remoteOffset + data->remoteBufferSize;
}

CompressedIPPacket::CompressedIPPacket(const CompressedIPPacket &old) : data(old.data) {
    data->count++;
}

CompressedIPPacket &CompressedIPPacket::operator=(CompressedIPPacket &&other) noexcept {
    if (&other != this) {
        if (data && !--data->count) {
            delete data->mBuffer;
            delete data;
        }
        data = other.data;
        other.data = nullptr;
    }
}

CompressedIPPacket &CompressedIPPacket::operator=(const CompressedIPPacket &other) {
    if (&other != this) {
        if (data && !--data->count) {
            delete data->mBuffer;
            delete data;
        }
        data = other.data;
        data->count++;
    }
}

CompressedIPPacket::CompressedIPPacket(CompressedIPPacket &&old) noexcept: data(old.data) {
    old.data = nullptr;
}

void CompressedIPPacket::setRemoteBuffer(char *buf, unsigned short len, unsigned short offset) {
    if (offset > data->mBufferSize)throw out_of_range("Invalid remote buffer offset");
    if (!data->mBuffer && len < getIPHeaderSize())
        throw length_error(
                "Remote buffer capacity of ip packet with no internal buffer can not be less than " +
                to_string(getIPHeaderSize()));
    data->remoteOffset = offset;
    data->remoteBuffer = buf;
    data->remoteBufferSize = len;
}

char *CompressedIPPacket::getRemoteBuffer() const {
    return data->remoteBuffer;
}

unsigned short CompressedIPPacket::getRemoteBufferSize() const {
    return data->remoteBufferSize;
}

char *CompressedIPPacket::getBuffer() const {
    return data->mBuffer;
}

bool CompressedIPPacket::isValid() { return true; }

void CompressedIPPacket::validate() {}

void CompressedIPPacket::setProtocol(unsigned short proto) {
    getIPHeader().protocol = proto;
}

unsigned short CompressedIPPacket::writeTo(void *buf, unsigned short size) {
    auto remaining = size;
    if (data->mBuffer) {
        auto amt = min(data->remoteOffset, (unsigned short) remaining);
        memcpy(buf, data->mBuffer, amt);
        remaining -= amt;
    }
    if (remaining && data->remoteBuffer) {
        auto amt = min(data->remoteBufferSize, (unsigned short) remaining);
        memcpy((char *) buf + data->remoteOffset, data->remoteBuffer,
               amt);
        remaining -= amt;
    }
    return size - remaining;
}

unsigned short CompressedIPPacket::readFrom(void *buf, unsigned short size) {
    auto remaining = size;
    if (data->mBuffer) {
        auto amt = min(data->remoteOffset, (unsigned short) remaining);
        memcpy(data->mBuffer, buf, amt);
        remaining -= amt;
    }
    if (remaining && data->remoteBuffer) {
        auto amt = min(data->remoteBufferSize, (unsigned short) remaining);
        memcpy(data->remoteBuffer, (char *) buf + data->remoteOffset,
               amt);
        remaining -= amt;
    }
    return size - remaining;
}

template<typename Object>
unsigned short CompressedIPPacket::readFromObject(Object &obj) {
    readFrom(&obj, sizeof obj);
}


#endif //TUNSERVER_COMPRESSED_IPPACKET_H

#pragma clang diagnostic pop