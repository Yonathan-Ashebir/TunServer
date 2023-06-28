//
// Created by yoni_ash on 6/25/23.
//

#ifndef TUNSERVER_DISPOSABLETUNNEL_H
#define TUNSERVER_DISPOSABLETUNNEL_H

#include "../Include.h"
#include "CompressedTunnel.h"

class DisposableTunnel : public CompressedTunnel {
public:

    inline explicit DisposableTunnel(TCPSocket &sock);;

    bool writePacket(CompressedIPPacket &packet) override;

    bool readPacket(CompressedIPPacket &packet) override;

    bool isGood();

    void pingRemote();

};

DisposableTunnel::DisposableTunnel(TCPSocket &sock) : CompressedTunnel(sock) {

}


#endif //TUNSERVER_DISPOSABLETUNNEL_H
