//
// Created by yoni_ash on 4/27/23.
//

#include "TCPConnection.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

TCPConnection::TCPConnection(Tunnel &tunnel, int &maxFd, fd_set *rcv, fd_set *snd, fd_set *err) : tunnel(tunnel),
                                                                                                  maxFd(maxFd) {
    if (!(rcv && snd && err))throw invalid_argument("Null set is not allowed");
    receiveSet = rcv;
    sendSet = snd;
    errorSet = err;
}

TCPConnection::~TCPConnection() {
    delete sendBuffer;
    ::printf("Connection destroyed");
}

int createTcpSocket() {
    int result = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);//warn: might not work on windows
    return result;
}

void TCPConnection::receiveFromClient(TCPPacket &packet) {
    auto generateSequenceNo = []() {
        return rand() % 2 ^ 31;
    };

    sockaddr_in client = packet.getSource();
    sockaddr_in server = packet.getDestination();

    auto sendReset = [&packet, this] {
        packet.swapEnds();
        if (packet.isAck()) packet.makeResetSeq(packet.getAcknowledgmentNumber());
        else
            packet.makeResetAck(packet.getSequenceNumber() + packet.getSegmentLength());
        tunnel.writePacket(packet);
    };
    auto sendAck = [&packet, this] {
        packet.swapEnds();
        packet.clearData();
        packet.makeNormal(sendNext, receiveNext);
        tunnel.writePacket(packet);

    };
    if (packet.isSyn() && !packet.isAck()) {
        switch (state) {
            case CLOSED: {
                int sock = createTcpSocket();
                if (sock < 0) {
                    exitWithError("Could not create socket");//todo: take it simple
                }
                int res = connect(sock, (sockaddr *) &server, sizeof(sockaddr_in));
                if (res != 0 && res != EAGAIN && res != EINPROGRESS && res != EALREADY && res != EISCONN) {
                    sendReset();
                } else {
                    //re-establish
                    fd = sock;
                    sendNewDataSequence = sendSequence = sendUnacknowledged = sendNext = generateSequenceNo();
                    receiveSequence = receiveUser = receiveNext = packet.getSequenceNumber() + 1;
                    sendWindow = packet.getWindowSize();
                    windowShift = packet.getWindowShift();
                    mss = packet.getMSS();
                    clientReadFinished = false;
                    serverReadFinished = false;
                    lastAcknowledgeSequence = 0;
                    retryCount = 0;
                    auto now = chrono::steady_clock::now();
                    lastSendTime = lastAcknowledgmentSent = lastTimeAcknowledgmentAccepted = now;


                    const unsigned int newLen = (65535 << windowShift) * SEND_WINDOW_SCALE;
                    if (!sendBuffer || sendLength < newLen) {
                        delete sendBuffer;
                        sendBuffer = new unsigned char[newLen];
                        sendLength = newLen;
                    }

                    source = client;
                    destination = server;

                    if (res == 0 || res == EISCONN) {
                        packet.swapEnds();
                        packet.clearData();
                        packet.makeSyn(sendNext, receiveNext);
                        tunnel.writePacket(packet);
                        state = SYN_ACK_SENT;
                    } else
                        state = SYN_RECEIVED;
                }
                break;
            }
            case SYN_RECEIVED:
            case SYN_ACK_SENT: {
                if (packet.getSequenceNumber() != receiveNext - 1) {
                    sendReset();
                    break;
                }
            }
            default: {
                sendAck();
            }
        }
    } else if (packet.isSyn() && packet.isAck())sendReset();
    else if (packet.isReset()) {
        switch (state) {
#ifdef STRICT_MODE
            case CLOSED:
                break;
#endif
                //info: since there is no syn_sent state
            default: {
                unsigned int seq = packet.getSequenceNumber();
                if (seq >= receiveNext &&
                    seq < receiveNext + (getReceiveAvailable()))
                    closeConnection();
            }
        }

    } else {
        if (!packet.isAck())sendReset();

        else {
            unsigned int seq = packet.getSequenceNumber();
            unsigned int ack = packet.getAcknowledgmentNumber();
            switch (state) {
                case SYN_ACK_SENT: {
                    if (seq == receiveNext) {
#ifdef STRICT_MODE
                        if (ack != sendUnacknowledged + 1)exitWithError("Unexpected ack segment");
#endif
                        sendSequence = sendUnacknowledged = sendNewDataSequence = sendNext = ack;
                        auto now = chrono::steady_clock::now();
                        rtt = lastAcknowledgmentSent - now;
                        lastTimeAcknowledgmentAccepted = now;
                        FD_SET(fd, receiveSet);
                        maxFd = max(maxFd, fd);
                        state = ESTABLISHED; //todo: Another state change
                    } else sendReset();
                    break;
                }
                case ESTABLISHED: {
                    if (seq == receiveNext) {
                        if (!clientReadFinished) {
                            //If did not receive fin
                            unsigned int len = packet.getDataLength();
                            unsigned int available = getReceiveAvailable();

                            if (len > available) {
                                flushDataToServer(packet);
                                _trimReceiveBuffer();
                            }
                            available = getReceiveAvailable();
                            unsigned int total = packet.copyDataTo(receiveBuffer + receiveNext, available);
                            receiveNext += total;

                            if (packet.isFin() && total == len) {
                                //Process fin
                                receiveNext++;
                                clientReadFinished = true;//todo: state modifier here too
                            }
                            //Flush data
                            flushDataToServer(packet);
                        }

                        //Accept ack anyway
                        sendUnacknowledged = max(sendUnacknowledged, ack);
                        auto now = chrono::steady_clock::now();
                        auto d = lastTimeAcknowledgmentAccepted - now;
                        if (d < rtt / 2)lastSendTime += d;
                        lastTimeAcknowledgmentAccepted = now;
                        retryCount = 0;

                        //Both might be complete now
                        if (isUpStreamComplete() && isDownStreamComplete())closeConnection();
                        else {
                            //If they are not complete
                            trimSendBuffer();
                            acknowledgeDelayed(packet);
                        }

                    } else {
                        sendAck();
                    }
                    break;
                }
                default:
                    sendReset();
            }
        }
    }

}

void TCPConnection::receiveFromServer(TCPPacket &packet) {
    //Only matter if state == ESTABLISHED
    if (state != ESTABLISHED)return;
    trimSendBuffer();
    size_t total = read(fd, sendBuffer + sendNewDataSequence - sendSequence, getSendAvailable());
    if (total > 0) {
        //Advance next read point
        sendNewDataSequence += total;
    } else if (total == 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
        //All data has been received from server or assume so !!!
        if (!serverReadFinished) {
            sendNewDataSequence++;
            serverReadFinished = true;
        }
    }

    flushDataToClient(packet);
}


//Info: flush calls often should trim buffers, close the connection (or parts of it), handle data never flushed to node case (e.g. by using RESET packets);
void TCPConnection::flushDataToServer(TCPPacket &packet) {
    if (state != ESTABLISHED)return;

    if (clientReadFinished && receiveUser == receiveNext - 1) {
        receiveUser++;
        if (isDownStreamComplete())closeConnection();
        else if (canSendToServer()) shutdown(fd, SHUT_WR);
    } else {
        auto amt = receiveNext - receiveUser - (clientReadFinished ? 1 : 0);
        if (amt > 0) {
            size_t total = send(fd, receiveBuffer + receiveUser - receiveSequence, amt, 0);
            if (total > 0) {
                receiveUser += total;
                if (clientReadFinished && receiveUser == receiveNext - 1) {
                    receiveUser++;
                    if (isDownStreamComplete())closeConnection();
                    else if (canSendToServer()) shutdown(fd, SHUT_WR);
                } else if (!clientReadFinished)
                    trimReceiveBuffer();
            } else if (total == 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
                //Socket's write terminated early
                if (total != 0 && (errno != EWOULDBLOCK && errno != EAGAIN))
                    printError("Error writing to server");
                else
                    printError("Early terminated socket's write");
                packet.setEnds(destination, source);
                packet.makeResetSeq(sendNext);
                tunnel.writePacket(packet);
                closeConnection();
            }
        }
    }
}


void TCPConnection::flushDataToClient(TCPPacket &packet) {
    if (state != ESTABLISHED)return;
    auto now = chrono::steady_clock::now();


    if (now - lastSendTime > rtt) {
        if (retryCount == SEND_MAX_RETRIES) {
            closeConnection();
            return;
        }
        auto amt = sendNewDataSequence - sendNext;
        if (amt == 0)return;
        for (auto seq = sendUnacknowledged; seq < sendNewDataSequence;) {
            packet.setEnds(destination, source);
            packet.makeNormal(seq, receiveNext);
            auto len = min(sendNewDataSequence - seq, (unsigned int) mss);
            packet.clearData();
            packet.appendData(sendBuffer + seq - sendSequence, len);
            tunnel.writePacket(packet);
            seq += len;
        }
        sendNext = sendNewDataSequence;
        lastTimeAcknowledgmentAccepted = lastSendTime = now;
        retryCount++;
    } else {
        auto amt = sendNewDataSequence - sendNext;
        if (amt < mss / 2)return;
        for (auto seq = sendNext; seq < sendNewDataSequence;) {
            packet.setEnds(destination, source);
            packet.makeNormal(seq, receiveNext);
            auto len = min(sendNewDataSequence - seq, (unsigned int) mss);
            packet.clearData();
            packet.appendData(sendBuffer + seq - sendSequence, len);
            tunnel.writePacket(packet);
            seq += len;
        }
        sendNext = sendNewDataSequence;
    }
    if (isDownStreamComplete() && isUpStreamComplete())closeConnection();
    else {
        acknowledgeDelayed(packet);
    }


}


#pragma clang diagnostic pop