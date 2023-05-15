//
// Created by yoni_ash on 4/27/23.
//

#include "TCPConnection.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

TCPConnection::TCPConnection(Tunnel &tunnel, socket_t &maxFd, fd_set *rcv, fd_set *snd, fd_set *err) : tunnel(tunnel),
                                                                                                       maxFd(maxFd) {
    if (!(rcv && snd && err))throw invalid_argument("Null set is not allowed");
    receiveSet = rcv;
    sendSet = snd;
    errorSet = err;
}

TCPConnection::~TCPConnection() {
    delete sendBuffer;
#ifdef LOGGING
    ::printf("Connection destroyed");
#endif
}

socket_t createTcpSocket() {
    socket_t result;
#ifdef _WIN32
    result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    result = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
#endif

//    int val = 1;
//    if (setsockopt(result, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
//        exitWithError("Could not disable Nagle algorithm");

    return result;
}

void TCPConnection::receiveFromClient(TCPPacket &packet) {
    auto generateSequenceNo = []() {
        return random() % 2 ^ 31;
    };

    sockaddr_in client = packet.getSource();
    sockaddr_in server = packet.getDestination();

    auto sendReset = [&packet, this] {
        packet.swapEnds();
        if (packet.isAck()) packet.makeResetSeq(packet.getAcknowledgmentNumber());
        else
            packet.makeResetAck(packet.getSequenceNumber() + packet.getSegmentLength());
        tunnel.writePacket(packet);
#ifdef LOGGING
        ::printf("Sent reset from receiveFromClient\n");
#endif
    };
    auto sendAck = [&packet, this] {
        packet.swapEnds();
        packet.makeNormal(sendNext, receiveNext);
        packet.setWindowSize((unsigned short) (getReceiveAvailable()));
        packet.clearData();
        tunnel.writePacket(packet);
#ifdef LOGGING
        ::printf("Sent ack from receiveFromClient\n");
#endif
    };
    if (packet.isSyn() && !packet.isAck()) {
        switch (state) {
            case CLOSED: {
                socket_t sock = createTcpSocket();
                if (sock < 0) {
                    exitWithError("Could not create socket");//todo: take it simple
                }
                int res = connect(sock, (sockaddr *) &server, sizeof(sockaddr_in));
                if (res != 0 && errno != EAGAIN && errno != EINPROGRESS && errno != EALREADY && errno != EISCONN) {
                    sendReset();
                } else {
                    //re-establish
                    fd = sock;
                    sendNewDataSequence = sendSequence = sendUnacknowledged = sendNext = generateSequenceNo();
                    receiveSequence = receiveUser = receiveNext = receivePushSequence =
                            packet.getSequenceNumber() + 1;
                    sendWindow = packet.getWindowSize();
                    windowShift = packet.getWindowShift();
                    mss = packet.getMSS();
                    clientReadFinished = false;
                    serverReadFinished = false;
                    lastAcknowledgedSequence = 0;
                    retryCount = 0;
                    auto now = chrono::steady_clock::now();
                    lastSendTime = lastTimeAcknowledgmentAccepted = now;

                    const unsigned int newLen = (65535 << windowShift) * SEND_WINDOW_SCALE;
                    if (!sendBuffer || sendLength < newLen) {
                        delete sendBuffer;
                        sendBuffer = new BUFFER_BYTE[newLen];
                        sendLength = newLen;
                    }

                    source = client;
                    destination = server;

                    if (res == 0 || errno == EISCONN) {
                        packet.swapEnds();
                        packet.clearData();
                        packet.makeSyn(sendNext, receiveNext);
                        tunnel.writePacket(packet);
                        state = SYN_ACK_SENT;
#ifdef LOGGING
                        ::printf("Sent syn_ack\n");
#endif

                    } else
                        state = SYN_RECEIVED;
                }
                break;
            }
            case SYN_RECEIVED:
            case SYN_ACK_SENT: {
                auto seq = packet.getSequenceNumber();
                if (seq == receiveNext - 1) {
                    packet.swapEnds();
                    packet.clearData();
                    packet.makeSyn(sendNext, receiveNext);
                    tunnel.writePacket(packet);
                    state = SYN_ACK_SENT;
#ifdef LOGGING
                    ::printf("Sent syn_ack again\n");
#endif
                } else
                    sendReset();
                break;
            }
            default: {
                sendAck();
            }
        }
    } else if (packet.isSyn() && packet.isAck())sendReset();
    else if (packet.isReset()) {
        switch (state) {
            case CLOSED:
                break;

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
                        rtt = min(now - lastSendTime, MAX_RTT);
                        lastTimeAcknowledgmentAccepted = now;
                        FD_SET(fd, receiveSet);
                        maxFd = max(maxFd, fd + 1);
                        state = ESTABLISHED; //todo: Another state change
#ifdef LOGGING
                        ::printf("Received ack and set rtt = %ld\n",rtt.count() / 1000000);
#endif
                    } else sendReset();
                    break;
                }
                case ESTABLISHED: {
                    if (seq == receiveNext) {
                        if (seq >= receiveNext) {
#ifdef LOGGING
                            if (ack <= sendUnacknowledged)
                                ::printf("Accepted acknowledgment(%d) <= sendUnacknowledged(%d)\n", ack,
                                         sendUnacknowledged);
#endif
//                            const auto space = sendUnacknowledged - sendSequence;
//                            const auto available = getSendAvailable();
//                            if (available > 0) {
//                                printf("Send buffer was not filled well when an acknowledgement was accepted: %d/%d\n",
//                                       available, sendLength);
//                            }
//                            if (available <= 2 * mss && space >= mss) {
//                                printf("Space(%d) could be re-used and added to send buffer(%d) but have not been\n",
//                                       space, available);
//                            }
                            sendUnacknowledged = max(sendUnacknowledged, ack);
                            retryCount = 0;
                        }
                        if (!clientReadFinished) {
                            //If did not receive fin
                            unsigned int len = packet.getDataLength();
                            unsigned int available = getReceiveAvailable();

                            if (len > available) {
                                flushDataToServer(packet);
                                _trimReceiveBuffer();
                            }
                            available = getReceiveAvailable();

                            unsigned int total = packet.copyDataTo(receiveBuffer + receiveNext - receiveSequence,
                                                                   available);
                            receiveNext += total;
                            if (packet.isPush()) {
                                receivePushSequence = receiveNext;
#ifdef LOGGING
                                ::printf("Push flag set in the datagram received & isFin %b\n", packet.isFin());
#endif
                            }
#ifdef LOGGING
                            if (len > 0) {
                                printf("Accepted data packet: %d\n", total);
                            } else ::printf("Accepted simple data ack\n");
#endif


                            if (packet.isFin() && total == len) {
#ifdef LOGGING
                                ::printf("Client read finished\n");
#endif
                                //Process fin
                                receiveNext++;
                                clientReadFinished = true;//todo: state modifier here too
                            }
                            //Flush data
                            flushDataToServer(packet);
                            if (state == CLOSED)return;
                        }
#ifdef LOGGING
                        else ::printf("Accepted simple data ack\n");
#endif
                        //Both might be complete now
                        if (isUpStreamComplete() && isDownStreamComplete())closeConnection();
                        else {
                            //If they are not complete
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

/*Returns whether it can receive more*/
bool TCPConnection::receiveFromServer(TCPPacket &packet) {
    //Only matter if state == ESTABLISHED
    if (state != ESTABLISHED)return false;
    trimSendBuffer();
    auto amt = getSendAvailable();
    if (amt == 0)return false;
    int total = (int) recv(fd, reinterpret_cast<char *>(sendBuffer + sendNewDataSequence - sendSequence), (int) amt,
                           0);//warn:not logically safe
    if (total > 0 && total <= amt) {
        //Advance next read point
        sendNewDataSequence += total;
#ifdef LOGGING
        ::printf("Received %d bytes from server\n", total);
#endif
    } else if (amt > 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) {
        //All data has been received from server or assume so !!!
        if (!serverReadFinished) {
#ifdef LOGGING
            ::printf("Server read finished\n");
#endif
            //This is important as it a closed stream, will always be readable
            FD_CLR(fd, receiveSet);
            sendNewDataSequence++;
            serverReadFinished = true;

        }
    }

    flushDataToClient(packet);
    return getSendAvailable() > 0;
}


//Info: flush calls often should trim buffers, close the connection (or parts of it), handle data never flushed to node case (e.g. by using RESET packets);
void TCPConnection::flushDataToServer(TCPPacket &packet) {
    if (state != ESTABLISHED)return;

    if (isUpStreamComplete())return;
    if (clientReadFinished && receiveUser == receiveNext - 1) {
        receiveUser++;
        if (isDownStreamComplete())closeConnection();
        else shutdown(fd, SHUT_WR);
    } else {
        auto amt = receiveNext - receiveUser - (clientReadFinished ? 1 : 0);
        if (amt > 0) {
            int val;
//            if (receivePushSequence > receiveUser &&
//                setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
//                exitWithError("Could not disable Nagle algorithm");
//            }
            int total = (int) send(fd, reinterpret_cast<const char *>(receiveBuffer + receiveUser - receiveSequence),
                                   (int) amt,
                                   0);//todo: make sure using int is enough
            val = 0;
#ifdef  LOGGING
            ::printf("Flushed %d bytes to server\n", total);
#endif
//            if (receivePushSequence > receiveUser &&
//                setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
//                exitWithError("Could not re-enable Nagle algorithm");
//            }
            if (total > 0) {
                receiveUser += total;
                if (clientReadFinished && receiveUser == receiveNext - 1) {
                    receiveUser++;
                    if (isDownStreamComplete())closeConnection();
                    else shutdown(fd, SHUT_WR);
                } else if (!clientReadFinished)
                    trimReceiveBuffer();
            } else if (total == 0 || (errno != EWOULDBLOCK && errno != EAGAIN)) {
                //Socket's write terminated early
                if (total != 0 && (errno != EWOULDBLOCK && errno != EAGAIN))
                    printError("Error writing to server");
                else
                    printError("Early terminated write to upstream");
                packet.setEnds(destination, source);
                packet.makeResetSeq(sendNext);
                tunnel.writePacket(packet);
#ifdef LOGGING
                ::printf("Failed to flush it all to the server. So, sent reset\n");
#endif
                closeConnection();
            }
        }
    }
}


void TCPConnection::flushDataToClient(TCPPacket &packet) {
    if (state != ESTABLISHED)return;
    auto now = chrono::steady_clock::now();
    auto diff = now - lastSendTime;

    if (isDownStreamComplete())return;
    auto sendDataStartingFrom = [this, &packet](
            unsigned int seq) {//todo: check if this type of declaration has overhead
        packet.setEnds(destination, source);
        while (seq < sendNewDataSequence) {
            packet.makeNormal(seq, receiveNext);
            packet.setWindowSize((unsigned short) (getReceiveAvailable()));
            auto len = min(sendNewDataSequence - seq - (serverReadFinished ? 1 : 0), (unsigned int) mss);
#ifdef STRICT_MODE
            if (len > mss || sendNewDataSequence - seq == 0)
                exitWithError(("Packet data len to send is wrong: " + to_string(len)).c_str());
#endif
            packet.clearData();
            packet.appendData(sendBuffer + seq - sendSequence, len);
            seq += len;

            if (serverReadFinished && seq == sendNewDataSequence - 1) {
#ifdef LOGGING
                printf("Setting fin flag\n");
#endif
                packet.setFinFlag(true);
                seq += 1;
            }
            tunnel.writePacket(packet);
        }
    };
    //todo: this seems to inefficiently send new data that did not accumulate to mss
    if (sendUnacknowledged < sendNewDataSequence && diff.count() / 4 > rtt.count()) {
        if (retryCount == SEND_MAX_RETRIES) {
            closeConnection();
            return;
        }
        if (sendUnacknowledged < sendNext)retryCount++;
        sendDataStartingFrom(sendUnacknowledged);
#ifdef LOGGING
        ::printf("Sent unacknowledged data to the client due to timeout, rtt millis: %ld\n",
                 rtt.count() / 1000000);
#endif

        sendNext = sendNewDataSequence;
        lastTimeAcknowledgmentAccepted = lastSendTime = now;
    } else if (serverReadFinished && sendNext < sendNewDataSequence || sendNewDataSequence - sendNext > mss) {
        sendDataStartingFrom(sendNext);
#ifdef LOGGING
        ::printf("Sent new data to the client\n");
#endif

        sendNext = sendNewDataSequence;
        lastTimeAcknowledgmentAccepted = lastSendTime = now;
    }


    if (isUpStreamComplete())
        closeConnection();
//    acknowledgeDelayed(packet);
}


#pragma clang diagnostic pop