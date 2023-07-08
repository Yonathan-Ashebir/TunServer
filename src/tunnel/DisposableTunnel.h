//
// Created by yoni_ash on 6/25/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_DISPOSABLETUNNEL_H
#define TUNSERVER_DISPOSABLETUNNEL_H

#include <utility>

#include "../Include.h"
#include "CompressedTunnel.h"

class DisposableTunnel : public CompressedTunnel {
public:
    struct UnsentData {
        char *buffer;
        unsigned int size;
        unsigned int offset;
        unsigned int remaining;

        friend class DisposableTunnel;

    public:
        UnsentData(char *buf, unsigned int size, unsigned int off, unsigned int rem) : buffer(buf), size(size),
                                                                                       offset(off), remaining(rem) {}

        [[nodiscard]] unsigned int getRemaining() const { return remaining; }
    };

    using OnNewTunnelResponse = function<void(sockaddr_storage &)>;
    using OnDeleteTunnelRequest = function<void(unsigned int)>;
    using OnUnsentData = function<void(UnsentData &data)>;
    using OnNewTunnelRequest = function<void(void)>;

    inline explicit DisposableTunnel(TCPSocket &sock, fd_set &rcv, fd_set &snd, fd_set &err,
                                     unsigned int size = MIN_SS * 5) : CompressedTunnel(sock),
                                                                       data(new Data(size, rcv, snd, err)) {
        socket.setIn(rcv);
        socket.setIn(err);
        data->lastSuccessfulSendTime = chrono::steady_clock::now();
    };

    bool writePacket(CompressedIPPacket &packet) override {
        if (!isOpen())return false;
        auto totalSize = 1 + sizeof(DataSegment) + packet.getLength() +
                         1; //The last '1' is for the space reserved by ping segment
        auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                   data->sendUser;
        auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                       data->lastPacketSent;
        if (totalSize > available) {
            flushPackets();
            currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                  data->sendUser;
            available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                      data->lastPacketSent;
            if (totalSize > available)return false;
        }
        data->incompleteSend = true;

        if (!currentBufferSize) {
            data->sendWrappedAround = true;
            currentBufferSize = data->lastPacketSent;
            data->sendUser = 0;
        }
        data->sendBuffer[data->sendUser++] = DATA_SEGMENT;
        currentBufferSize--;


        auto amt = min((unsigned int) sizeof(DataSegment), currentBufferSize);
        DataSegment segment{toNetworkByteOrder(packet.getLength())};
        memcpy(data->sendBuffer + data->sendUser, &segment, amt);
        auto remaining = sizeof(DataSegment) - amt;
        if (remaining) {
            data->sendWrappedAround = true;
            currentBufferSize = data->lastPacketSent;
            memcpy(data->sendBuffer, (char *) &segment + amt, remaining);
            data->sendUser = remaining;
        } else {
            data->sendUser += sizeof(DataSegment);
            currentBufferSize -= sizeof(DataSegment);
        }

        remaining = packet.getLength();
        amt = packet.writeTo(data->sendBuffer + data->sendUser, currentBufferSize);
        remaining -= amt;
        if (remaining) {
            data->sendWrappedAround = true;
            data->sendUser = remaining;
            packet.writeTo(data->sendBuffer, remaining);
        } else data->sendUser += packet.getLength();
        socket.setIn(data->sendSet);
        return true;
    };

    void flushPackets() {
        if (!isOpen() || (!data->sendWrappedAround && data->sendNext == data->sendUser))return;
        auto sock = getTCPSocket();
        try {
            auto currentBufferSize = data->sendWrappedAround ? data->size - data->sendNext : data->sendUser -
                                                                                             data->sendNext;
            auto res = sock.sendIgnoreWouldBlock(data->sendBuffer + data->sendNext,
                                                 (int) currentBufferSize);
            if (res > 0) {
                data->lastSuccessfulSendTime = chrono::steady_clock::now();
                data->sendNext += res;
            }
            if (res < currentBufferSize) {
                if (res > 0)
                    advanceLastPacketSent();
                return;
            }

            if (data->sendWrappedAround) {
                res = sock.sendIgnoreWouldBlock(data->sendBuffer,
                                                (int) data->sendUser);
                if (res > 0) {
                    data->sendNext = res;
                }
                if (res < data->sendUser) {
                    if (res > 0)
                        advanceLastPacketSent();
                    return;
                }
            }
            advanceLastPacketSent();

            data->incompleteSend = false;
            data->lastPacketSent = data->sendUser = data->sendNext = 0;
            data->sendWrappedAround = false;
            socket.unsetFrom(data->sendSet);
            return;
        } catch (SocketError &e) {

#ifdef LOGGING
            cout << "Tunnel socket flush failed with error: \n   what(): " << e.what() << endl;
#endif
            close();
        }

    }

    vector<CompressedIPPacket> readPackets(unsigned int max = numeric_limits<unsigned int>::max()) {
        vector<CompressedIPPacket> result{};
        if (!isOpen())return result;

        bool wrappedAround = false;
        unsigned int dispatchedStart;
        if (data->receiveType == 0) assert(data->receiveNext == data->receiveDispatched);
        if (data->receiveNext == data->receiveDispatched) {
            dispatchedStart = 0;
            data->receiveNext = 0;
            data->receiveDispatched = 0;
        } else {
            dispatchedStart = data->receiveDispatched;
        }

        while (true) {
            switch (data->receiveType) {
                case 0: {
                    auto &sock = getTCPSocket();
                    unsigned int currentBufferSize = wrappedAround ? dispatchedStart - data->receiveNext : data->size -
                                                                                                           data->receiveNext;

                    if (currentBufferSize < 1) {
                        if (wrappedAround)return result;
                        wrappedAround = true;
                        data->receiveNext = 0;
                        data->receiveDispatched = 0;
                        if (dispatchedStart < 1)return result;
                        currentBufferSize = dispatchedStart;
                    }
                    try {
                        auto res = sock.receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext, 1);
                        if (res > 0) {
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            char t = data->receiveBuffer[data->receiveDispatched];
                            data->receiveNext++;
                            data->receiveDispatched++;
                            currentBufferSize--;
                            data->incompleteReceive = true;
                            data->receiveType = t;
                            unsigned int headerSize{};
                            if (t == DATA_SEGMENT)headerSize = sizeof(DataSegment);
                            else if (t == NEW_TUNNEL_RESPONSE_SEGMENT)
                                headerSize = sizeof(NewTunnelResponseSegment) + sizeof(in6_addr);
                            else if (t == DELETE_TUNNEL_SEGMENT)
                                headerSize = sizeof(DeleteTunnelSegment);

                            if (currentBufferSize < headerSize) {
                                if (wrappedAround)return result;
                                wrappedAround = true;
                                data->receiveNext = 0;
                                data->receiveDispatched = 0;
                                if (dispatchedStart < headerSize)return result;
                            }
                        } else return result;
                    } catch (SocketError &e) {
#ifdef LOGGING
                        cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                        close();
                        return result;
                    }
                    break;
                }
                case PING_SEGMENT: {
                    auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser :
                                             data->size -
                                             data->sendUser;
                    auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                                   data->lastPacketSent;
                    if (!available) break;
                    if (!currentBufferSize) {
                        data->sendWrappedAround = true;
                        data->sendUser = 0;
                    }
                    data->sendBuffer[data->sendUser++] = PING_RESPONSE_SEGMENT;
                    data->receiveType = 0;
                    data->incompleteReceive = false;
                    break;
                }
                case PING_RESPONSE_SEGMENT: {
                    data->hasSentPing = false;
                    data->receiveType = 0;
                    data->incompleteReceive = false;
                    break;
                }
                case DATA_SEGMENT: {
                    if (data->receiveRemaining == 0) {
                        try {
                            auto remaining = sizeof(DataSegment) - data->receiveNext + data->receiveDispatched;
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) remaining);
                            if (res < 1)return result;
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            data->receiveNext += res;

                            if (res != remaining) return result;
                            auto size = toHostByteOrder(((DataSegment *) (data->receiveBuffer +
                                                                          data->receiveDispatched))->size);
                            if (!isValidPacketSize(size)) {
                                close();
                                return result;
                            }
                            data->receiveDispatched += sizeof(DataSegment);
                            data->receiveRemaining = size;

                            unsigned int currentBufferSize = wrappedAround ? dispatchedStart - data->receiveNext :
                                                             data->size - data->receiveNext;
                            if (currentBufferSize < size) {
                                if (wrappedAround)return result;
                                wrappedAround = true;
                                data->receiveNext = 0;
                                data->receiveDispatched = 0;
                                if (dispatchedStart < size)return result;
                            }
                        } catch (SocketError &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    } else {
                        try {
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) data->receiveRemaining);
                            if (res < 1)return result;
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            data->receiveNext += res;
                            data->receiveRemaining -= res;
                            if (data->receiveRemaining) return result;
                            result.emplace_back(data->receiveBuffer + data->receiveDispatched,
                                                data->receiveNext - data->receiveDispatched);
                            data->receiveType = 0;
                            data->receiveDispatched = data->receiveNext;
                            data->incompleteReceive = false;
                            if (result.size() == max)return result;
                        } catch (SocketError &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    }
                    break;
                }
                case NEW_TUNNEL_REQUEST_SEGMENT: {
                    data->onNewTunnelRequest();
                    data->receiveType = 0;
                    data->incompleteReceive = false;
                    break;
                }
                case NEW_TUNNEL_RESPONSE_SEGMENT: {
                    if (data->receiveRemaining == 0) {
                        try {
                            auto remaining =
                                    sizeof(NewTunnelResponseSegment) - data->receiveNext + data->receiveDispatched;
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) remaining);
                            if (res < 0) return result;
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            data->receiveNext += res;
                            if (res != remaining) return result;
                            auto family = ((NewTunnelResponseSegment *) (data->receiveBuffer +
                                                                         data->receiveDispatched))->family;
                            if (family == AF_INET)
                                data->receiveRemaining = sizeof(in_addr);
                            else if (family == AF_INET6)
                                data->receiveRemaining = sizeof(in6_addr);
                            else {
                                close();
                                return result;
                            }
                        } catch (SocketError &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    } else {
                        try {
                            auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                              (int) data->receiveRemaining);
                            if (res < 1) return result;
                            data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                            data->receiveNext += res;
                            data->receiveRemaining -= res;
                            if (data->receiveRemaining) return result;

                            /*IMP: the tunnel should validate the address*/
                            auto &header = *(NewTunnelResponseSegment *) (data->receiveBuffer +
                                                                          data->receiveDispatched);
                            sockaddr_storage address{header.family};
                            if (header.family == AF_INET) {
                                auto ain = reinterpret_cast<sockaddr_in *>(&address);
                                ain->sin_port = header.port;
                                memcpy(&ain->sin_addr, data->receiveBuffer + data->receiveDispatched +
                                                       sizeof(NewTunnelResponseSegment), sizeof(in_addr));
                            } else {
                                /*Since we know it is ipv6 for sure, if not ipv4*/
                                auto ain = reinterpret_cast<sockaddr_in6 *>(&address);
                                ain->sin6_port = header.port;
                                memcpy(&ain->sin6_addr, data->receiveBuffer + data->receiveDispatched +
                                                        sizeof(NewTunnelResponseSegment), sizeof(in6_addr));
                            }
                            data->onNewTunnelResponse(address);

                            data->receiveType = 0;
                            data->receiveDispatched = data->receiveNext;
                            data->incompleteReceive = false;
                        } catch (SocketError &e) {
#ifdef LOGGING
                            cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                            close();
                            return result;
                        }
                    }
                    break;
                }
                case DELETE_TUNNEL_SEGMENT: {
                    auto alreadyRead = data->receiveNext - data->receiveDispatched;
                    try {
                        auto remaining = sizeof(DeleteTunnelSegment) - alreadyRead;
                        auto res = getTCPSocket().receiveIgnoreWouldBlock(data->receiveBuffer + data->receiveNext,
                                                                          (int) remaining);
                        if (res < 1) return result;
                        data->lastSuccessfulReceiveTime = chrono::steady_clock::now();
                        data->receiveNext += res;
                        if (res != remaining) return result;
                        auto &header = *reinterpret_cast<DeleteTunnelSegment *>(data->receiveBuffer +
                                                                                data->receiveDispatched);
                        data->onDeleteTunnelRequest(toHostByteOrder(header.id));

                        data->receiveType = 0;
                        data->receiveDispatched = data->receiveNext;
                        data->incompleteReceive = false;
                    } catch (SocketError &e) {
#ifdef LOGGING
                        cout << "Tunnel's read packet failed with error\n   what(): " << e.what() << endl;
#endif
                        close();
                        return result;
                    }
                    break;
                }
                default: {
#ifdef LOGGING
                    cout << "Encountered an odd segment type: " << data->receiveType << endl;
#endif
                    close();
                    return result;
                }
            }
        }
    }

    bool readPacket(CompressedIPPacket &packet) override {
        if (!isOpen())
            return false;
        auto res = readPackets(1);
        if (res.empty())
            return false;
        packet = res.at(0);
        return true;
    }

    bool isOpen() {
        return data->isOpen;
    }

    void close() {
        if (isOpen()) {
            data->isOpen = false;
            socket.unsetFrom(data->receiveSet);
            socket.unsetFrom(data->sendSet);
            socket.unsetFrom(data->errorSet);
            socket.close();

            if (data->sendNext != data->sendUser) {
                UnsentData unsentData{data->sendBuffer, data->size, data->lastPacketSent,
                                      (data->sendWrappedAround) ? data->size - data->lastPacketSent + data->sendUser :
                                      data->sendUser - data->lastPacketSent};
                data->onUnsentData(unsentData);
#ifdef LOGGING
                cout << "The Socket was closed and " << unsentData.getRemaining() << " bytes were not resent" << endl;
#endif
            }
        }
    }

    void reset(TCPSocket &sock) {
        reset(sock, data->receiveSet, data->sendSet, data->errorSet);
    }

    void reset(TCPSocket &sock, fd_set &rcv, fd_set &snd, fd_set &err) {
        close();
        socket = sock;
        data->receiveSet = rcv;
        data->sendSet = snd;
        data->errorSet = err;

        data->sendUser = data->sendNext = data->lastPacketSent = 0;
        data->sendWrappedAround = data->incompleteSend = false;
        data->lastSuccessfulSendTime = chrono::steady_clock::now();

        data->receiveNext = data->receiveDispatched = data->receiveRemaining = 0;
        data->receiveType = 0;
        data->incompleteReceive = false;

        data->hasSentPing = false;

        data->isOpen = true;
    }

    void checkStatus() {
        if (!isOpen())return;
        if (data->incompleteSend && chrono::steady_clock::now() - data->lastSuccessfulSendTime > sendTimeout) {
#ifdef LOGGING
            cout << "Tunnel's send timed-out" << endl;
#endif
            close();
            return;
        }

        if (data->incompleteReceive && chrono::steady_clock::now() - data->lastSuccessfulReceiveTime > receiveTimeout) {
#ifdef LOGGING
            cout << "Tunnel's receive timed out" << endl;
#endif
            close();
            return;
        }

        if (data->hasSentPing) {
            if (chrono::steady_clock::now() - data->lastPingTime > pingTimeout) {
#ifdef LOGGING
                cout << "Tunnel ping timed out" << endl;
#endif
                close();
            }
        } else {
            /*send ping*/
            auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                       data->sendUser;
            auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                           data->lastPacketSent;
            if (available < 2) return;
            if (!currentBufferSize) {
                data->sendWrappedAround = true;
                data->sendUser = 0;
            }
            data->sendBuffer[data->sendUser++] = PING_SEGMENT;
            data->incompleteSend = true;
            data->hasSentPing = true;
            data->lastPingTime = chrono::steady_clock::now();
        }
    }

    bool sendNewTunnelRequest() {
        if (!isOpen())return false;
        auto totalSize = 2;
        auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                   data->sendUser;
        auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                       data->lastPacketSent;
        if (totalSize > available) {
            flushPackets();
            currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                  data->sendUser;
            available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                      data->lastPacketSent;
            if (totalSize > available)return false;
        }
        data->incompleteSend = true;

        if (!currentBufferSize) {
            data->sendWrappedAround = true;
            data->sendUser = 0;
        }

        data->sendBuffer[data->sendUser++] = NEW_TUNNEL_REQUEST_SEGMENT;
        return true;
    }

    bool sendNewTunnelResponse(sockaddr_storage &addr) {
        if (!isOpen())return false;

        if (addr.ss_family == AF_INET) {
            auto totalSize = 1 + sizeof(NewTunnelResponseSegment) + sizeof(in_addr) + 1;
            auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                       data->sendUser;
            auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                           data->lastPacketSent;
            if (totalSize > available) {
                flushPackets();
                currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                      data->sendUser;
                available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                          data->lastPacketSent;
                if (totalSize > available)return false;
            }
            data->incompleteSend = true;

            if (!currentBufferSize) {
                data->sendWrappedAround = true;
                data->sendUser = 0;
            }
            data->sendBuffer[data->sendUser++] = NEW_TUNNEL_RESPONSE_SEGMENT;
            currentBufferSize--;

            auto amt = min(currentBufferSize, (unsigned int) sizeof(NewTunnelResponseSegment));
            auto addrIn = reinterpret_cast<sockaddr_in *>(&addr);
            NewTunnelResponseSegment header{addrIn->sin_port, addr.ss_family};
            memcpy(data->sendBuffer + data->sendUser, &header, amt);
            if (sizeof(NewTunnelResponseSegment) > amt) {
                auto remaining = sizeof(NewTunnelResponseSegment) - amt;
                data->sendUser = remaining;
                data->sendWrappedAround = true;
                currentBufferSize = data->lastPacketSent;
                memcpy(data->sendBuffer, (char *) &header + amt, remaining);
            } else {
                data->sendUser += sizeof(NewTunnelResponseSegment);
                currentBufferSize -= sizeof(NewTunnelResponseSegment);
            }

            amt = min(currentBufferSize, (unsigned int) sizeof(in_addr));
            memcpy(data->sendBuffer + data->sendUser, &addrIn->sin_addr, amt);
            if (amt < sizeof(in_addr)) {
                auto remaining = sizeof(in_addr) - amt;
                data->sendUser = remaining;
                data->sendWrappedAround = true;
                memcpy(data->sendBuffer, (char *) &addrIn->sin_addr + amt, remaining);
            } else {
                data->sendUser += sizeof(NewTunnelResponseSegment);
            }
        } else if (addr.ss_family == AF_INET6) {
            auto totalSize = 1 + sizeof(NewTunnelResponseSegment) + sizeof(in6_addr) + 1;
            auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                       data->sendUser;
            auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                           data->lastPacketSent;
            if (totalSize > available) {
                flushPackets();
                currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                      data->sendUser;
                available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                          data->lastPacketSent;
                if (totalSize > available)return false;
            }

            if (!currentBufferSize) {
                data->sendWrappedAround = true;
                data->sendUser = 0;
            }
            data->sendBuffer[data->sendUser++] = NEW_TUNNEL_RESPONSE_SEGMENT;
            currentBufferSize--;

            auto amt = min(currentBufferSize, (unsigned int) sizeof(NewTunnelResponseSegment));
            auto addrIn6 = reinterpret_cast<sockaddr_in6 *>(&addr);
            NewTunnelResponseSegment response{addrIn6->sin6_port, addr.ss_family};
            memcpy(data->sendBuffer + data->sendUser, &response, amt);
            if (sizeof(NewTunnelResponseSegment) > amt) {
                auto remaining = sizeof(NewTunnelResponseSegment) - amt;
                data->sendUser = remaining;
                data->sendWrappedAround = true;
                currentBufferSize = data->lastPacketSent;
                memcpy(data->sendBuffer, (char *) &response + amt, remaining);
            } else {
                data->sendUser += sizeof(NewTunnelResponseSegment);
                currentBufferSize -= sizeof(NewTunnelResponseSegment);
            }

            amt = min(currentBufferSize, (unsigned int) sizeof(in6_addr));
            memcpy(data->sendBuffer + data->sendUser, &addrIn6->sin6_addr, sizeof(in6_addr));
            if (amt < sizeof(in6_addr)) {
                auto remaining = sizeof(in6_addr) - amt;
                data->sendUser = remaining;
                data->sendWrappedAround = true;
                memcpy(data->sendBuffer, (char *) &addrIn6->sin6_addr + amt, remaining);
            } else {
                data->sendUser += sizeof(NewTunnelResponseSegment);
            }
        } else {
            throw invalid_argument("Unknown address family");
        }
        return true;
    }

    bool sendDeleteTunnelRequest(unsigned short id) {
        if (!isOpen())return false;

        auto totalSize = 1 + sizeof(DeleteTunnelSegment) + 1;
        auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                   data->sendUser;
        auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                       data->lastPacketSent;
        if (totalSize > available) {
            flushPackets();
            currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                  data->sendUser;
            available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                      data->lastPacketSent;
            if (totalSize > available)return false;
        }
        data->incompleteSend = true;

        if (!currentBufferSize) {
            data->sendWrappedAround = true;
            data->sendUser = 0;
        }
        data->sendBuffer[data->sendUser++] = DELETE_TUNNEL_SEGMENT;
        currentBufferSize--;

        auto amt = min(currentBufferSize, (unsigned int) sizeof(DeleteTunnelSegment));
        DeleteTunnelSegment segment{toNetworkByteOrder(id)};
        memcpy(data->sendBuffer + data->sendUser, &segment, amt);
        if (sizeof(DeleteTunnelSegment) > amt) {
            auto remaining = sizeof(DeleteTunnelSegment) - amt;
            data->sendUser = remaining;
            data->sendWrappedAround = true;
            memcpy(data->sendBuffer, (char *) &segment + amt, remaining);
        } else {
            data->sendUser += sizeof(DeleteTunnelSegment);
        }

        return true;
    }

    unsigned int sendUnsentData(UnsentData &unsentData) {
        if (!isOpen())return 0;
        if (!unsentData.remaining)return 0;

        auto currentBufferSize = data->sendWrappedAround ? data->lastPacketSent - data->sendUser : data->size -
                                                                                                   data->sendUser;
        auto available = data->sendWrappedAround ? currentBufferSize : currentBufferSize +
                                                                       data->lastPacketSent;
        unsigned int result{};
        while (unsentData.remaining) {
            if (unsentData.offset == unsentData.size) unsentData.offset = 0;
            auto t = unsentData.buffer[unsentData.offset];
            unsigned int totalSize = 1;
            switch (t) {
                case NEW_TUNNEL_RESPONSE_SEGMENT: {
                    auto amt = min((unsigned int) sizeof(NewTunnelResponseSegment),
                                   unsentData.size - unsentData.offset - 1);
                    NewTunnelResponseSegment header{};
                    memcpy(&header, unsentData.buffer + unsentData.offset + 1, amt);
                    if (amt < sizeof(NewTunnelResponseSegment)) {
                        memcpy((char *) &header + amt, unsentData.buffer, sizeof(NewTunnelResponseSegment) - amt);
                    }
                    totalSize += sizeof(NewTunnelResponseSegment) +
                                 (header.family == AF_INET ? sizeof(in_addr) : sizeof(in6_addr));
                    break;
                }
                case DATA_SEGMENT: {
                    auto amt = min((unsigned int) sizeof(DataSegment),
                                   unsentData.size - unsentData.offset - 1);
                    DataSegment header{};
                    memcpy(&header, unsentData.buffer + unsentData.offset + 1, amt);
                    if (amt < sizeof(DataSegment)) {
                        memcpy((char *) &header + amt, unsentData.buffer, sizeof(DataSegment) - amt);
                    }
                    totalSize += sizeof(DataSegment) + toHostByteOrder(header.size);
                    break;
                }
                case DELETE_TUNNEL_SEGMENT: {
                    totalSize += sizeof(DeleteTunnelSegment);
                    break;
                }
                case PING_SEGMENT:
                case PING_RESPONSE_SEGMENT:
                case NEW_TUNNEL_REQUEST_SEGMENT:
                    break;
                default:
                    throw logic_error("Unknown segment type: " + to_string(t));
            }
            if (totalSize > available)return result;
            data->incompleteSend = true;

            unsigned int total = min(totalSize, unsentData.size - unsentData.offset);
            auto amt = min(total, currentBufferSize);
            memcpy(data->sendBuffer + data->sendUser, unsentData.buffer + unsentData.offset, amt);
            unsentData.offset += amt;
            if (amt < total) {
                auto remaining = total - amt;
                data->sendUser = remaining;
                data->sendWrappedAround = true;
                currentBufferSize = data->lastPacketSent;
                memcpy(data->sendBuffer, unsentData.buffer + unsentData.offset, remaining);
                unsentData.offset += remaining;
            } else {
                data->sendUser += total;
                currentBufferSize -= total;
            }

            if (total < totalSize) {
                total = totalSize - total;
                amt = min(total, currentBufferSize);
                memcpy(data->sendBuffer + data->sendUser, unsentData.buffer, amt);
                if (amt < total) {
                    auto remaining = total - amt;
                    data->sendUser = remaining;
                    data->sendWrappedAround = true;
                    currentBufferSize = data->lastPacketSent;
                    memcpy(data->sendBuffer, unsentData.buffer + amt, remaining);
                } else {
                    data->sendUser += total;
                    currentBufferSize -= total;
                }
                unsentData.offset = total;
            }

            unsentData.remaining -= totalSize;
            result += totalSize;
        }
        return result;
    }

    void setId(unsigned id) {
        data->id = id;
    }

    unsigned int getId() {
        return data->id;
    }

    OnNewTunnelResponse &getOnNewTunnelResponse() {
        return data->onNewTunnelResponse;
    }

    void setOnNewTunnelResponse(OnNewTunnelResponse callback) {
        data->onNewTunnelResponse = std::move(callback);
    }

    OnNewTunnelRequest &getOnNewTunnelRequest() {
        return data->onNewTunnelRequest;
    }

    void setOnNewTunnelRequest(OnNewTunnelRequest callback) {
        data->onNewTunnelRequest = std::move(callback);
    }

    OnDeleteTunnelRequest &getOnDeleteTunnelRequest() {
        return data->onDeleteTunnelRequest;
    }

    void setOnDeleteTunnelRequest(OnDeleteTunnelRequest callback) {
        data->onDeleteTunnelRequest = std::move(callback);
    }

    void setOnUnsentData(OnUnsentData callback) {
        data->onUnsentData = std::move(callback);
    }

    OnUnsentData &getOnUnsentData() {
        return data->onUnsentData;
    }

private:

    struct Data {
        unsigned int size;
        char *sendBuffer = new char[size]{};
        unsigned int sendUser{};
        unsigned int sendNext{};
        bool sendWrappedAround{};
        unsigned int lastPacketSent{};
        bool incompleteSend{};
        chrono::steady_clock::time_point lastSuccessfulSendTime{};

        char *receiveBuffer = new char[size]{};
        unsigned int receiveNext{};
        unsigned int receiveDispatched{};
        char receiveType{};
        unsigned int receiveRemaining{};
        bool incompleteReceive{};
        chrono::steady_clock::time_point lastSuccessfulReceiveTime{};

        OnNewTunnelResponse onNewTunnelResponse{[](sockaddr_storage &) {}};
        OnNewTunnelRequest onNewTunnelRequest{[] {}};
        OnDeleteTunnelRequest onDeleteTunnelRequest{[](unsigned int) {}};
        OnUnsentData onUnsentData{[](UnsentData &) {}};


        fd_set &receiveSet;
        fd_set &sendSet;
        fd_set &errorSet;

        unsigned int id{};
        bool isOpen{true};

        bool hasSentPing{};
        chrono::time_point<chrono::steady_clock, chrono::nanoseconds> lastPingTime{};

        ~Data() {
            delete[] sendBuffer;
            delete[] receiveBuffer;
        }

        Data(unsigned int size, fd_set &rcv, fd_set &snd, fd_set &err) : size(size), receiveSet(rcv), sendSet(snd),
                                                                         errorSet(err) {

        }
    };

    shared_ptr<Data> data;

    constexpr static chrono::milliseconds pingTimeout{2000};
    constexpr static chrono::milliseconds sendTimeout{2000};
    constexpr static chrono::milliseconds receiveTimeout{2000};

    void advanceLastPacketSent() {
        while (data->lastPacketSent != data->sendNext) {
            unsigned int totalSize{1};
            auto t = data->sendBuffer[data->lastPacketSent];
            switch (t) {
                case DATA_SEGMENT: {
                    auto currentBuf = data->size - data->lastPacketSent - 1;
                    auto amt = min(currentBuf, (unsigned int) sizeof(DataSegment));
                    DataSegment header{};
                    memcpy(&header, data->sendBuffer + data->lastPacketSent + 1, amt);
                    if (amt < sizeof(DataSegment)) {
                        memcpy((char *) &header + amt, data->sendBuffer, sizeof(DataSegment) - amt);
                    }
                    totalSize += sizeof(DataSegment) + toHostByteOrder(header.size);
                    break;
                }
                case NEW_TUNNEL_RESPONSE_SEGMENT: {
                    auto currentBuf = data->size - data->lastPacketSent - 1;
                    auto amt = min(currentBuf, (unsigned int) sizeof(NewTunnelResponseSegment));
                    NewTunnelResponseSegment header{};
                    memcpy(&header, data->sendBuffer + data->lastPacketSent + 1, amt);
                    if (amt < sizeof(NewTunnelResponseSegment)) {
                        memcpy((char *) &header + amt, data->sendBuffer, sizeof(NewTunnelResponseSegment) - amt);
                    }
                    totalSize += sizeof(NewTunnelResponseSegment) +
                                 (header.family == AF_INET ? sizeof(in_addr) : sizeof(in6_addr));
                    break;
                }
                case DELETE_TUNNEL_SEGMENT: {
                    totalSize += sizeof(DeleteTunnelSegment);
                    break;
                }
                case PING_SEGMENT:
                case PING_RESPONSE_SEGMENT:
                case NEW_TUNNEL_REQUEST_SEGMENT: {
                    break;
                }
                default: {
                    throw logic_error("Segment not implemented");
                }
            }
            auto available =
                    data->sendNext > data->lastPacketSent ? data->sendNext - data->lastPacketSent : data->size -
                                                                                                    data->lastPacketSent +
                                                                                                    data->sendNext;
            if (totalSize > available)break;
            data->lastPacketSent = (data->size - data->lastPacketSent > totalSize) ? data->lastPacketSent + totalSize :
                                   totalSize + data->lastPacketSent - data->size;
        }
    }

    TCPSocket &getTCPSocket() {
        return *reinterpret_cast<TCPSocket *>(&socket);
    }

    inline void receivedPing() {

    }

};

#endif //TUNSERVER_DISPOSABLETUNNEL_H

#pragma clang diagnostic pop