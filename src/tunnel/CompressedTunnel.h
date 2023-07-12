//
// Created by DELL on 6/28/2023.
//

#ifndef TUNSERVER_COMPRESSEDTUNNEL_H
#define TUNSERVER_COMPRESSEDTUNNEL_H

#include "../Include.h"
#include "../packet/CompressedIPPacket.h"

class CompressedTunnel {
public:
    inline explicit CompressedTunnel(Socket
                                     &sock) :
            socket(sock) {}

    virtual bool writePacket(CompressedIPPacket &packet) = 0;

    virtual bool readPacket(CompressedIPPacket &packet) = 0;

    [[nodiscard]] inline Socket & getSocket() { return socket; }

    Socket socket;
};

#endif //TUNSERVER_COMPRESSEDTUNNEL_H
