//
// Created by DELL on 6/30/2023.
//

#ifndef TUNSERVER_CONSTANTS_H
#define TUNSERVER_CONSTANTS_H

#include "./helpers/Include.h"

struct DataSegment {
    unsigned short size;
};

struct NewTunnelResponseSegment {
    unsigned short port;
    sa_family_t family;
};


struct DeleteTunnelSegment {
    unsigned short id;
};

#define DATA_SEGMENT 1
#define PING_SEGMENT 2
#define PING_RESPONSE_SEGMENT 3
#define NEW_TUNNEL_REQUEST_SEGMENT 4
#define NEW_TUNNEL_RESPONSE_SEGMENT 5
#define DELETE_TUNNEL_SEGMENT 6

#define MIN_PACKET_SIZE 20
#define MAX_PACKET_SIZE (576*4)

constexpr bool isValidSegmentType(char t) {
    return 0 < t && t < 7;
}

constexpr bool isValidPacketSize(unsigned short size) {
    return MIN_PACKET_SIZE < size && size < MAX_PACKET_SIZE;
}

#endif //TUNSERVER_CONSTANTS_H
