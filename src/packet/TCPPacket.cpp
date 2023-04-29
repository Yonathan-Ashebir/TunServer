//
// Created by yoni_ash on 4/21/23.
//


#include "TCPPacket.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
using namespace std;

const unsigned int OPTIONS_INDEX = sizeof(iphdr) + sizeof(tcphdr);

TCPPacket::TCPPacket(unsigned int size) : Packet(size) {
    tcphdr *tcph = (tcphdr *) buffer + sizeof(iphdr);


    tcph->seq = htonl(0);
    tcph->ack_seq = htonl(0);
    tcph->doff = 10; // tcp header size
    tcph->window = htons(5840); // window size: large one seen = 64240
}

tcphdr *TCPPacket::getTcpHeader() {
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

void TCPPacket::setOption(unsigned char kind, unsigned char len, unsigned char *payload) {
    if (kind < 2)throw invalid_argument("Can not set option with kind: " + to_string(kind));

    unsigned int ind;
    for (ind = OPTIONS_INDEX; ind < OPTIONS_INDEX + optionsLength;) {
        unsigned int k;
        switch (k = buffer[ind]) {
            case 0:
                goto end;
            case 1:
                ind++;
                break;
            default: {
                unsigned int l = buffer[ind + 1];
                if (kind == k) {
                    if (len != l)
                        throw invalid_argument(
                                "Option " + to_string(kind) + " has already been set with length " + to_string(l) +
                                "and can not be set to " + to_string(len));
                    memcpy(buffer + ind + 2, payload, len - 2);
                    return;
                }
                ind += l;
            }
        }
    }
    end:
    unsigned int headerEnd = OPTIONS_INDEX + optionsLength;
    if (headerEnd < ind + len) {
        unsigned int diff = (ind + len) - headerEnd;
        if (diff % 4 > 0) diff = (diff / 4) * 4 + 4;
        diff = max(20u, diff);

        if (length + diff > maxSize) {
            throw invalid_argument("Did not decide whether to grow size on demand");
        } else {
            unsigned int dataLen = length - headerEnd;
            unsigned char temp[dataLen];
            memcpy(temp, buffer + headerEnd, dataLen);
            memset(buffer + headerEnd, 0, diff);
            memcpy(buffer + headerEnd + diff, temp, dataLen);
        }
        optionsLength += diff;
        length += diff;
    }
    buffer[ind] = kind;
    buffer[ind + 1] = len;
    memcpy(buffer + ind + 2, payload, len - 2);
}

bool TCPPacket::removeOption(unsigned char kind) {
    if (kind < 2)throw invalid_argument("Can not set option with kind: " + to_string(kind));
    unsigned int ind;
    for (ind = OPTIONS_INDEX; ind < OPTIONS_INDEX + optionsLength;) {
        unsigned int k;
        switch (k = buffer[ind]) {
            case 0:
                return false;
            case 1:
                ind++;
                break;
            default: {
                unsigned int l = buffer[ind + 1];
                if (kind == k) {
                    if (buffer[ind + l] > 0) {
                        unsigned int trailingOptions = optionsLength - (ind - OPTIONS_INDEX) - l;
                        shiftElements(buffer + ind + l, trailingOptions,
                                      -l); // NOLINT(cppcoreguidelines-narrowing-conversions)
                    }
                    return true;
                }
                ind += l;
            }
        }
    }
    return false;
}

sockaddr_in TCPPacket::getSource() {//warn: skipped memset to zero
    auto tcph = getTcpHeader();
    auto iph = getIpHeader();
    sockaddr_in addr{AF_INET, tcph->source, iph->saddr};
    return addr;
}

bool TCPPacket::isFrom(sockaddr_in &src) {
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

bool TCPPacket::isSyn() {
    auto tcph = getTcpHeader();
    return tcph->syn;
}

bool TCPPacket::isAck() {
    auto tcph = getTcpHeader();
    return tcph->ack;
}

bool TCPPacket::isUrg() {
    auto tcph = getTcpHeader();
    return tcph->urg;
}

bool TCPPacket::isFin() {
    auto tcph = getTcpHeader();
    return tcph->fin;
}

bool TCPPacket::isReset() {
    auto tcph = getTcpHeader();
    return tcph->rst;
}

bool TCPPacket::isPush() {
    auto tcph = getTcpHeader();
    return tcph->psh;
}

unsigned short TCPPacket::getWindowSize() {
    auto tcph = getTcpHeader();
    return ntohs(tcph->window);
}

unsigned char TCPPacket::getOption(unsigned char kind, unsigned char *data, unsigned char len) {
    if (kind < 2)throw invalid_argument("Can not set option with kind: " + to_string(kind));
    unsigned int ind;
    for (ind = OPTIONS_INDEX; ind < OPTIONS_INDEX + optionsLength;) {
        unsigned int k;
        switch (k = buffer[ind]) {
            case 0:
                return 0;
            case 1:
                ind++;
                break;
            default: {
                unsigned int l = buffer[ind + 1];
                if (kind == k) {
                    if (len < l)
                        throw invalid_argument(
                                "Option data buffer length not enough, needed: " + to_string(l) + " available: " +
                                to_string(len));
                    memcpy(data, buffer + ind, l);
                    return l;
                }
                ind += l;
            }
        }
    }
    return 0;
}

inline unsigned char TCPPacket::getWindowShift() {
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

unsigned int TCPPacket::available() const {
    return maxSize - length;
}

unsigned int TCPPacket::getDataLength() {
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

inline unsigned int TCPPacket::getSegmentLength() {
    unsigned int result = getDataLength();
    if (isSyn())result++;
    if (isFin())result++;
    return result;
}

#pragma clang diagnostic pop