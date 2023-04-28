//
// Created by yoni_ash on 4/27/23.
//

#include "TCPConnection.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

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
                    fd = sock;
                    sendSequence = sendUnacknowledged = sendNext = generateSequenceNo();
                    receiveSequence = receiveUser = receiveNext = packet.getSequenceNumber() + 1;
                    sendWindow = packet.getWindowSize();
                    windowShift = packet.getWindowShift();
                    mss = packet.getMSS();

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
                    close();
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
                        sendSequence = ack;
                        sendUnacknowledged = ack;
                        sendNext = ack;
                        state = ESTABLISHED; //todo: Another state change
                    } else sendReset();
                    break;
                }
                case ESTABLISHED: {
                    if (seq == receiveNext) {
                        unsigned int len = packet.getDataLength();
                        unsigned int available = getReceiveAvailable();

                        if (len > available) {
                            flushDataToServer();
                            trimReceiveBuffer();
                        }
                        available = getReceiveAvailable();
                        unsigned int total = packet.copyDataTo(receiveBuffer + receiveNext, available);
                        receiveNext += total;

                        flushDataToServer();
                        sendUnacknowledged = max(sendUnacknowledged, ack);
                        if (packet.isFin() && total == len) {
                            receiveNext++;
                            state = CLOSE_WAIT;//todo: state modifier here too

                        }
                        acknowledgeDelayed(packet);
                    } else {
                        sendAck();
                    }
                }
                case CLOSE_WAIT: {
                    if (seq == receiveNext && packet.getDataLength() == 0) {
                        sendUnacknowledged = max(sendUnacknowledged, ack);
                    } else sendAck();
                    break;
                }
                case LAST_ACK: {
                    if (seq == receiveNext && packet.getDataLength() == 0) {
                        sendUnacknowledged = max(sendUnacknowledged, ack);
                        if (sendUnacknowledged == sendNext && receiveUser == receiveNext)close();
                    } else sendAck();
                    break;
                }
                case FIN_WAIT1:
                case FIN_WAIT2: {
                    if (seq == receiveNext) {
                        unsigned int len = packet.getDataLength();
                        unsigned int available = getReceiveAvailable();

                        if (len > available) {
                            flushDataToServer();
                            trimReceiveBuffer();
                        }
                        available = getReceiveAvailable();
                        unsigned int total = packet.copyDataTo(receiveBuffer + receiveNext, available);
                        receiveNext += total;

                        flushDataToServer();
                        sendUnacknowledged = max(sendUnacknowledged, ack);
                        if (sendUnacknowledged == sendNext) {
                            state = FIN_WAIT2;//todo: state modifier here too
                        }
                        if (packet.isFin() && total == len) {
                            receiveNext++;
                            if (state == FIN_WAIT2) {
                                sendAck();
                            } else {
                                state = CLOSING;
                            }
                        }
                        acknowledgeDelayed(packet);
                    } else sendAck();
                    break;
                }
                case CLOSING: {
                    if (seq == receiveNext) {
                        sendUnacknowledged = max(sendUnacknowledged, ack);
                        if (sendUnacknowledged == sendNext && receiveUser == receiveNext)close();
                    } else sendAck();
                    break;
                }
                default:
                    sendReset();
            }
        }
    }

}

TCPConnection::states TCPConnection::getState() {
    return state;
}

TCPConnection::TCPConnection(Tunnel &tunnel, fd_set *rcv, fd_set *snd, fd_set *err) : tunnel(tunnel) {
    if (!(rcv && snd && err))throw invalid_argument("Null set is not allowed");
    receiveSet = rcv;
    sendSet = snd;
    errorSet = err;
}

void TCPConnection::close() {
    //todo: might affect opposite stream
    FD_CLR(fd, receiveSet);
    FD_CLR(fd, sendSet);
    FD_CLR(fd, errorSet);
    ::close(fd);
}


constexpr unsigned int TCPConnection::getReceiveAvailable() const {
    return RECEIVE_BUFFER_SIZE - receiveNext + receiveSequence;
}

void TCPConnection::acknowledgeDelayed(TCPPacket &packet) {
    auto now = chrono::steady_clock::now();
    if (chrono::duration_cast<chrono::milliseconds>(now - lastAcknowledgeTime).count() > ACKNOWLEDGE_DELAY ||
        receiveNext - lastAcknowledgeSequence > mss * 2) {
        packet.setEnds(destination, source);
        packet.clearData();
        packet.makeNormal(sendNext, receiveNext);
        tunnel.writePacket(packet);
        lastAcknowledgeTime = now;
        lastAcknowledgeSequence = receiveNext;
    }
}

void TCPConnection::closeUpStream() {
    shutdown(fd, SHUT_WR);
}

bool TCPConnection::isUpStreamOpen() {
    char buf;
    size_t res = send(fd, &buf, 0, 0);
    return res == 0;
}

bool TCPConnection::isDownStreamOpen() {
    char buf;
    size_t res = read(fd, &buf, 0);
    return res == 0;
}

void TCPConnection::receiveFromServer(TCPConnection &packet) {

}


#pragma clang diagnostic pop