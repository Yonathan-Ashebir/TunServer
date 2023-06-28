//
// Created by yoni_ash on 4/27/23.
//

#include "TCPSession.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

TCPSession::TCPSession(Tunnel &tunnel, socket_t &maxFd, fd_set &rcv, fd_set &snd, fd_set &err) : tunnel(tunnel),
                                                                                                 receiveSet(rcv),
                                                                                                 sendSet(snd),
                                                                                                 errorSet(err),
                                                                                                 maxFd(maxFd) {
}

TCPSession::~TCPSession() {
    delete sendBuffer;
#ifdef LOGGING
    ::printf("TCP session destroyed");
#endif
}


void TCPSession::receiveFromClient(TCPPacket &packet) {
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
        ::printf("Sent regenerate from receiveFromClient\n");
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
                TCPSocket sock;
                auto res = sock.tryConnect(server);
                if (res != 0 && !isConnectionInProgress()) {
                    sendReset();
                } else {
                    //re-establish
                    mSock = sock;
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
                        sendBuffer = new char[newLen];
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
                    closeSession();
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
                        if (ack != sendUnacknowledged + 1) {
                            sendReset();
                            break;
                        }
                        sendSequence = sendUnacknowledged = sendNewDataSequence = sendNext = ack;
                        auto now = chrono::steady_clock::now();
                        rtt = max(min(now - lastSendTime, MAX_RTT), chrono::nanoseconds(0));//todo: improve
                        lastTimeAcknowledgmentAccepted = now;
                        mSock.setIn(receiveSet);
                        maxFd = max(maxFd, mSock.getFD() + 1);
                        state = ESTABLISHED;
#ifdef LOGGING
                        ::printf("Received ack and set rtt = %ld\n", rtt.count() / 1000000);
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
                                clientReadFinished = true;
                            }
                            //Flush data
                            flushDataToServer(packet);
                            if (state == CLOSED)return;
                        }
#ifdef LOGGING
                        else ::printf("Accepted simple data ack\n");
#endif
                        //Both might be complete now
                        if (isUpStreamComplete() && isDownStreamComplete())closeSession();
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
bool TCPSession::receiveFromServer(TCPPacket &packet) {
    //Only matter if state == ESTABLISHED
    if (state != ESTABLISHED)return false;
    trimSendBuffer();
    auto amt = getSendAvailable();
    if (amt == 0)return false;
    int total = mSock.tryReceive(sendBuffer + sendNewDataSequence - sendSequence, (int) amt);//warn:not logically safe
    if (total != -1 && total != 0) {
        //Advance next read point
        sendNewDataSequence += total;
#ifdef LOGGING
        ::printf("Received %d bytes from server\n", total);
#endif
    } else if (!isWouldBlock()) {
        //All data has been received from server or assume so !!!
        if (!serverReadFinished) {
#ifdef LOGGING
            ::printf("Server read finished\n");
#endif
            //This is important as it a closed stream, will always be readable
            mSock.unsetFrom(receiveSet);
            sendNewDataSequence++;
            serverReadFinished = true;

        }
    }

    flushDataToClient(packet);
    return getSendAvailable() > 0;
}


//Info: flush calls often should trim buffers, close the session (or parts of it),
// handle data never flushed to node cases (e.g., by using RESET packets);
void TCPSession::flushDataToServer(TCPPacket &packet) {
    if (state != ESTABLISHED)return;

    if (isUpStreamComplete())return;
    if (clientReadFinished && receiveUser == receiveNext - 1) {
        receiveUser++;
        if (isDownStreamComplete())closeSession();
        else mSock.shutdownWrite();
    } else {
        auto amt = receiveNext - receiveUser - (clientReadFinished ? 1 : 0);
        if (amt > 0) {
//            if (receivePushSequence > receiveUser)mSock.enableNagle(false);
            int total = mSock.send(receiveBuffer + receiveUser - receiveSequence, (int) amt);

#ifdef  LOGGING
            ::printf("Flushed %d bytes to server\n", total);
#endif
//            if (receivePushSequence > receiveUser)mSock.enableNagle(true);
            if (total != -1) {
                receiveUser += total;
                if (clientReadFinished && receiveUser == receiveNext - 1) {
                    receiveUser++;
                    if (isDownStreamComplete())closeSession();
                    else mSock.shutdownWrite();
                } else if (!clientReadFinished)
                    trimReceiveBuffer();
            } else if (!isWouldBlock()) {
                //Socket's write terminated early
                if (isWouldBlock())
                    printError("Early terminated write to upstream");
                else
                    printError("Error writing to server");
                packet.setEnds(destination, source);
                packet.makeResetSeq(sendNext);
                tunnel.writePacket(packet);
#ifdef LOGGING
                ::printf("Failed to flush it all to the server. So, sent regenerate\n");
#endif
                closeSession();
            }
        }
    }
}


void TCPSession::flushDataToClient(TCPPacket &packet) {
    if (state != ESTABLISHED)return;
    auto now = chrono::steady_clock::now();
    auto diff = now - lastSendTime;

    if (isDownStreamComplete())return;
    auto sendDataStartingFrom = [this, &packet](
            unsigned int seq) {
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
    if (sendUnacknowledged < sendNewDataSequence && diff / 4 > rtt) {
        if (retryCount == SEND_MAX_RETRIES) {
            closeSession();
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
        closeSession();
//    acknowledgeDelayed(packet);
}


#pragma clang diagnostic pop