//
// Created by yoni_ash on 4/29/23.
//

#include "DatagramTunnel.h"

bool DatagramTunnel::writePacket(Packet &packet) {
    auto len = packet.getMaxSize();
    auto buffer = getDataBuffer();
    int res = send(fd, buffer, packet.getLength());
    return res == 0;
}

bool DatagramTunnel::readPacket(Packet &packet) {
    auto len = packet.getMaxSize();
    auto buffer = getDataBuffer();
    int total = recv(fd, buffer, len);
    if (total == len) { printError("Might have dropped something"); }
    if (!packet.checkValidity()) return false;
    else return true;
}

DatagramTunnel::DatagramTunnel(int fd) : Tunnel(fd) {

}
