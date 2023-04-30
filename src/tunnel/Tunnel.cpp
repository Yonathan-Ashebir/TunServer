//
// Created by yoni_ash on 4/22/23.
//

#include "Tunnel.h"

int Tunnel::getFileDescriptor() const { return fd; }

Tunnel::Tunnel(int fd) : fd(fd) {

}

unsigned char *Tunnel::getDataBuffer(Packet &packet) {

}



