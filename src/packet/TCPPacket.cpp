//
// Created by yoni_ash on 4/21/23.
//


#include "TCPPacket.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
using namespace std;


TCPPacket::TCPPacket(unsigned int size) : Packet(size) {
    tcphdr *tcph = (tcphdr *) buffer + sizeof(iphdr);
    getIpHeader()->protocol = IPPROTO_TCP;

    tcph->seq = htonl(0);
    tcph->ack_seq = htonl(0);
    tcph->doff = 10; // tcp header size
    tcph->window = htons(5840); // window size: large one seen = 64240
}


void TCPPacket::setOption(unsigned char kind, unsigned char len, unsigned char *payload) {
    if (kind < 2)throw invalid_argument("Can not set an option with kind: " + to_string(kind));

    const unsigned short OPTIONS_INDEX = getIpHeaderLength() + sizeof(tcphdr);
    unsigned int HEADER_END = getIpHeaderLength() + getTcpOptionsLength();

    unsigned int ind;
    for (ind = OPTIONS_INDEX; ind < HEADER_END;) {
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
    if (HEADER_END < ind + len) {
        unsigned short diff = (ind + len) - HEADER_END;
        if (diff % 4 > 0) diff = (diff / 4) * 4 + 4;
        diff = max((unsigned short) 20u, diff);

        if (getLength() + diff > maxSize) {
            throw invalid_argument("Did not decide whether to grow size on demand");
        } else {
            unsigned int dataLen = getDataLength();
            unsigned char temp[dataLen];
            memcpy(temp, buffer + HEADER_END, dataLen);
            memset(buffer + HEADER_END, 0, diff);
            memcpy(buffer + HEADER_END + diff, temp, dataLen);
        }
        setTcpOptionsLength(getTcpOptionsLength()+diff);
        setLength(getLength()+diff);
    }
    buffer[ind] = kind;
    buffer[ind + 1] = len;
    memcpy(buffer + ind + 2, payload, len - 2);
}

bool TCPPacket::removeOption(unsigned char kind) {
    if (kind < 2)throw invalid_argument("Can not set an option with kind: " + to_string(kind));

    const unsigned short OPTIONS_INDEX = getIpHeaderLength() + sizeof(tcphdr);
    unsigned int HEADER_END = getIpHeaderLength() + getTcpOptionsLength();
    unsigned int ind;
    for (ind = OPTIONS_INDEX; ind < HEADER_END;) {
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
                        unsigned int trailingOptions = getTcpOptionsLength() - (ind - OPTIONS_INDEX) - l;
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


unsigned char TCPPacket::getOption(unsigned char kind, unsigned char *data, unsigned char len) {
    if (kind < 2)throw invalid_argument("Can not set an option with kind: " + to_string(kind));
    const unsigned short OPTIONS_INDEX = getIpHeaderLength() + sizeof(tcphdr);
    unsigned int HEADER_END = getIpHeaderLength() + getTcpOptionsLength();
    unsigned int ind;
    for (ind = OPTIONS_INDEX; ind < HEADER_END;) {
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

unsigned int sum(const unsigned char *buf, unsigned size) {
    unsigned sum = 0, i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        unsigned short word16 = *(unsigned short *) &buf[i];
        sum += word16;
    }

    /* Handle an odd-sized case */
    if (size & 1) {
        unsigned short word16 = (unsigned char) buf[i];
        sum += word16;
    }
    return sum;
}

unsigned short checksum(const unsigned char *buf, unsigned size) {
    unsigned sum = 0, i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        unsigned short word16 = *(unsigned short *) &buf[i];
        sum += word16;
    }

    /* Handle an odd-sized case */
    if (size & 1) {
        unsigned short word16 = (unsigned char) buf[i];
        sum += word16;
    }

    /* Fold to get the ones-complement result */
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

    /* Invert to get the negative in ones-complement arithmetic */
    return ~sum;
}


void TCPPacket::validate() {
    auto iph = getIpHeader();
    const auto ipHeaderSize = getIpHeaderLength();

    iph->id = random();
    iph->check = 0;
    iph->check = checksum(buffer, ipHeaderSize);

    auto tcph = getTcpHeader();
    tcph->check = 0;
    unsigned int tcpSum = sum(buffer + ipHeaderSize, getLength() - ipHeaderSize);
    pseudo_header psh{iph->saddr, iph->daddr, 0, IPPROTO_TCP, htons(getLength() - ipHeaderSize)};
    auto pshSum = sum((unsigned char *) &psh, sizeof psh);
    tcpSum += pshSum;
    while (tcpSum >> 16)tcpSum = (tcpSum & 0xFFFF) + (tcpSum >> 16);
    tcph->check = ~tcpSum;

}

bool TCPPacket::isValid() {
    auto iph = getIpHeader();
    const auto ipHeaderSize = iph->ihl * 4;
    if (getProtocol() != 6 || checksum(buffer, ipHeaderSize) != 0)return false;

    pseudo_header psh{iph->saddr, iph->daddr, 0, IPPROTO_TCP, htons(getLength() - ipHeaderSize)};
    auto tcpSum = sum((unsigned char *) &psh, sizeof psh);
    tcpSum += sum(buffer + ipHeaderSize, getLength() - ipHeaderSize);
    while (tcpSum >> 16 > 0) tcpSum = ((tcpSum & 0xFFFF) + (tcpSum >> 16));

    return tcpSum == 0xffff;

}


#pragma clang diagnostic pop