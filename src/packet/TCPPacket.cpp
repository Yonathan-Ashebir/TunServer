//
// Created by yoni_ash on 4/21/23.
//


#include "TCPPacket.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
using namespace std;



TCPPacket::TCPPacket(unsigned int size) : Packet(size) {
    tcphdr *tcph = (tcphdr *) buffer + sizeof(iphdr);


    tcph->seq = htonl(0);
    tcph->ack_seq = htonl(0);
    tcph->doff = 10; // tcp header size
    tcph->window = htons(5840); // window size: large one seen = 64240
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

auto sum = [](const unsigned char *buf, unsigned size) {
    unsigned sum = 0, i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        unsigned short word16 = *(unsigned short *) &buf[i];
        sum += word16;
    }

    /* Handle odd-sized case */
    if (size & 1) {
        unsigned short word16 = (unsigned char) buf[i];
        sum += word16;
    }
    return sum;
};

void TCPPacket::makeValid() {
    auto iph = getIpHeader();
    auto tcph = getTcpHeader();

    iph->check = 0;
    tcph->check = 0;
    iph->tot_len = htonl(length);//todo: check if htonl is necessary

    unsigned int commonSum = sum(buffer + sizeof(iphdr), length - sizeof(iphdr));

    pseudo_header psh{iph->saddr, iph->daddr, 0, IPPROTO_TCP, iph->tot_len};//todo: the same here

    auto pshSum = sum((unsigned char *) &psh, sizeof psh);
    auto iphSum = sum(buffer, sizeof(iphdr));

    unsigned int tempSum = iphSum + commonSum;
    iph->check = ~((tempSum & 0xFFFF) + (tempSum >> 16));

    tempSum = pshSum + commonSum;
    tcph->check = ~((tempSum & 0xFFFF) + (tempSum >> 16));
}

bool TCPPacket::checkValidity() {
    auto iph = getIpHeader();
    auto tcph = getTcpHeader();

    unsigned short oldIpCheckSum = iph->check;
    unsigned short oldTcpCheckSum = tcph->check;

    iph->check = 0;
    tcph->check = 0;//todo: check if htonl is necessary

    unsigned int commonSum = sum(buffer + sizeof(iphdr), length - sizeof(iphdr));

    pseudo_header psh{iph->saddr, iph->daddr, 0, IPPROTO_TCP, iph->tot_len};//todo: the same here

    auto pshSum = sum((unsigned char *) &psh, sizeof psh);
    auto iphSum = sum(buffer, sizeof(iphdr));

    unsigned int tempSum = iphSum + commonSum;
    auto newIpCheckSum = ~((tempSum & 0xFFFF) + (tempSum >> 16));

    tempSum = pshSum + commonSum;
    auto newTcpCheck = ~((tempSum & 0xFFFF) + (tempSum >> 16));

    iph->check = oldIpCheckSum;
    tcph->check = oldTcpCheckSum;

    return (oldTcpCheckSum == newTcpCheck && oldIpCheckSum == newIpCheckSum) &&
           length == ntohl(iph->tot_len);//todo: the same again

}


#pragma clang diagnostic pop