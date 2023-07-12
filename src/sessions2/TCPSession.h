//
// Created by DELL on 7/4/2023.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_TCPSESSION_H
#define TUNSERVER_TCPSESSION_H

#include <utility>

#include "../Include.h"
#include "../packet/CompressedTCPPacket.h"

class TCPSession {
public:
    enum states {
        CLOSED,
        SYN_RECEIVED,
        SYN_ACK_SENT,
        ESTABLISHED
    };

    constexpr static chrono::milliseconds MIN_RTT{50};
    constexpr static chrono::seconds SYN_TIMEOUT{30};
    constexpr static chrono::milliseconds SYN_ACK_DELAY{500};
    constexpr static chrono::seconds NO_PROBING_TIMEOUT{30};

    TCPSession(fd_set &rcv, fd_set &snd, fd_set &err) : data(new Data{rcv, snd, err}) {

    }

    inline states getState() {
        return data->state;
    }

    inline bool canHandle(CompressedTCPPacket &packet) {
        if (getState() == CLOSED) return true;
        //todo implement ipv6
        auto clientAddr = reinterpret_cast<sockaddr_in * >(&data->clientAddress);
        if (packet.getSourceIP() != clientAddr->sin_addr.s_addr) return false;
        if (packet.getSourcePort() != toHostByteOrder(clientAddr->sin_port))return false;

        auto serverAddr = reinterpret_cast<sockaddr_in * >(&data->serverAddress);
        if (packet.getDestinationIP() != serverAddr->sin_addr.s_addr)return false;
        if (packet.getDestinationPort() != toHostByteOrder(serverAddr->sin_port))return false;

        return true;
    }

    bool receiveFromClient(CompressedTCPPacket &packet, function<void(CompressedTCPPacket &)> sendBack) {
        if (!canHandle(packet))return false;

        sockaddr_storage clientAddr = packet.getSourceAddress();
        sockaddr_storage serverAddr = packet.getDestinationAddress();

        auto sendReset = [&] {
            packet.swapEnds();
            if (packet.isAck()) packet.makeResetSeq(packet.getAcknowledgmentNumber());
            else
                packet.makeResetAck(packet.getSequenceNumber() + packet.getSegmentLength());
            sendBack(packet);
#ifdef LOGGING
            ::printf("Sent rest from 'receiveFromClient'\n");
#endif
        };

        auto sendAck = [&, this] {
            packet.swapEnds();
            packet.makeNormal(data->sendUnacknowledged + data->sendNextOffset + data->sentFinToClient,
                              data->receiveSent + data->receiveBuffer.position() + data->receiveFromClientFinished);
            packet.setWindow(data->receiveBuffer.available());
            sendBack(packet);
#ifdef LOGGING
            ::printf("Sent ack from 'receiveFromClient'\n");
#endif
        };

        if (packet.isSyn()) {
            if (packet.isAck()) {
                sendReset();
                return true;
            }
            switch (data->state) {
                case CLOSED: {
                    TCPSocket sock;
                    try {
                        auto res = sock.connect(serverAddr, true);
                        if (packet.getWindowShift() > 14) {
                            sendReset();
                            break;
                        }
                        data->reset();
                        data->socket = sock;
                        data->clientAddress = clientAddr;
                        data->serverAddress = serverAddr;
                        data->socket.setIn(data->errorSet);
                        data->sendWindowShift = packet.getWindowShift();
                        data->sendWindowRightEdge = packet.getWindowSize() << packet.getWindowShift();

                        unsigned int seq = packet.getSequenceNumber();
                        data->receiveSent = seq + 1;

                        if (res == 0) {
                            packet.swapEnds();
                            packet.makeSyn(data->sendUnacknowledged,
                                           data->receiveSent);
                            data->lastTimeSentNewData = chrono::steady_clock::now();
                            sendBack(packet);
                            data->state = SYN_ACK_SENT;
#ifdef LOGGING
                            ::printf("Sent syn_ack\n");
#endif

                        } else {
                            data->state = SYN_RECEIVED;
                            data->socket.setIn(data->sendSet);
                        }
                    } catch (SocketError &e) {
                        sendReset();
                    }
                    break;
                }
                case SYN_RECEIVED: {
                    auto seq = packet.getSequenceNumber();
                    if (seq != data->receiveSent - 1) sendReset();
                    break;
                }
                case SYN_ACK_SENT: {
                    auto seq = packet.getSequenceNumber();
                    if (seq != data->receiveSent - 1) {
                        sendReset();
                        break;
                    }

                    packet.swapEnds();
                    packet.makeSyn(data->sendUnacknowledged, data->receiveSent);
                    /* info: not setting last sent time here*/
                    sendBack(packet);
                    data->state = SYN_ACK_SENT;
#ifdef LOGGING
                    ::printf("Sent syn_ack again\n");
#endif
                    break;
                }
                default:
                    sendAck();
            }
        } else if (packet.isReset()) {
            if (data->state != CLOSED) {
                unsigned int seq = packet.getSequenceNumber();
                auto receiveNextOff = data->receiveBuffer.position() + data->receiveFromClientFinished;
                auto off = seq - data->receiveSent;
                if (data->receiveBuffer.available() ? off >= receiveNextOff &&
                                                      off < receiveNextOff +
                                                            data->receiveBuffer.available() : off == receiveNextOff)
                    closeSession();
            }
        } else if (packet.isAck()) {
            auto seq = packet.getSequenceNumber();
            auto ack = packet.getAcknowledgmentNumber();

            switch (data->state) {
                case SYN_ACK_SENT: {
                    if (seq != data->receiveSent ||
                        ack != data->sendUnacknowledged) {
                        sendReset();
                        break;
                    }
                    data->sendUnacknowledged++;
                    auto now = chrono::steady_clock::now();
                    data->pushRTT(chrono::duration_cast<chrono::milliseconds>(data->lastTimeSentNewData - now));
                    data->lastTimeAcceptedAcknowledgement = now;
                    data->socket.setIn(data->receiveSet);
                    data->state = ESTABLISHED;
#ifdef LOGGING
                    cout << "Received ack and set rtt = " << data->getMeanRTT().count() << "ms" << endl;
#endif
                    break;
                }
                case ESTABLISHED: {
                    if (seq - data->receiveSent != data->receiveBuffer.position() + data->receiveFromClientFinished) {
#ifdef LOGGING
                        cout << "Out-of-order seq in the packet received, seq - receiveNext = "
                             << seq - data->receiveSent - data->receiveBuffer.position() -
                                data->receiveFromClientFinished
                             << endl;
                        if (ack - data->sendUnacknowledged < 0) {
                            cout << "(1) Old acknowledgement was received, sendUnacknowledged - ack = "
                                 << data->sendUnacknowledged - ack
                                 << endl;
                        }
#endif
                        sendAck();
                        break;
                    }

                    const auto ackOff = static_cast<int>(ack - data->sendUnacknowledged);
                    if (ackOff > data->sendNextOffset + data->sentFinToClient) {
                        sendAck();
                        break;
                    }

#ifdef LOGGING
                    if (ackOff < 0) {
                        cout << "(2) Old acknowledgement was received, sendUnacknowledged - ack = "
                             << data->sendUnacknowledged - ack
                             << endl;
                    }
#endif
                    auto now = chrono::steady_clock::now();
                    if (ackOff >= 0) {
                        data->lastTimeAcceptedAcknowledgement = now;
                        data->sendWindowRightEdge = packet.getWindowSize() << data->sendWindowShift;
                    }

                    auto amt = ackOff;
                    if (ackOff > data->sendNextOffset) {
                        /*this indicates an acknowledgement to the fin that has been sent*/
                        if (data->flushToServerFinished) {
                            closeSession();
                            break;
                        }
                        data->flushToClientFinished = true;
                        amt--;
                    }

                    if (amt > 0) {
                        data->sendUnacknowledged += amt;
                        data->sendNextOffset -= amt;
                        data->sendBuffer.compact(data->sendBuffer.position() - amt);
                        /*since we know that some space has been made available*/
                        data->socket.setIn(data->receiveSet);
                    }

                    if (data->rttTracking &&
                        data->sendUnacknowledged + data->flushToClientFinished - data->rttTrackedSendSequence >= 0) {
                        data->rttTracking = false;
                        data->pushRTT(chrono::duration_cast<chrono::milliseconds>(
                                now - data->rttSendTime));
                    }

                    if (data->receiveFromClientFinished) break;
                    auto len = min(static_cast<unsigned int>(packet.getTCPPayloadSize()),
                                   data->receiveBuffer.available());
                    if (len) {
                        data->receiveBuffer.put(packet.getBuffer() + packet.getTCPPayloadOffset(), len);
                        data->socket.setIn(data->sendSet);
#ifdef  LOGGING
                        cout << "Accepted " << len << " bytes from the client" << endl;
#endif
                    }

                    if (packet.isFin() && len == packet.getTCPPayloadSize()) {
                        data->receiveFromClientFinished = true;
                        sendAck();
                    } else acknowledgeDelayed(packet, sendBack);
                    break;
                }
                default:
                    sendReset();
            }
        } else
            sendReset();
        return true;
    }

    void receiveFromServer(CompressedTCPPacket &packet, function<void(CompressedTCPPacket &)> sendBack) {
        if (data->state != ESTABLISHED || data->receiveFromServerFinished)return;

        data->sendBuffer.manipulate([&](char *buf, unsigned int len) -> unsigned int {
            try {
                auto res = data->socket.receiveIgnoreWouldBlock(buf, (int) len);
                if (res == -1)return 0;
                if (res == 0) {
                    data->socket.unsetFrom(data->receiveSet);
                    data->receiveFromServerFinished = true;
                }
                return res;
            } catch (SocketError &e) {
#ifdef LOGGING
                cout << "Server's receive exited with an error: \n   " << e.what() << endl;
#endif
                data->socket.unsetFrom(data->receiveSet);
                data->receiveFromServerFinished = true;
                return 0;
            }
        });

        flushDataToClient(packet, std::move(sendBack));
        if (data->sendBuffer.available() == 0)
            data->socket.unsetFrom(data->receiveSet);
    }

    void flushDataToServer(CompressedTCPPacket &packet, function<void(CompressedTCPPacket &)> sendBack) {
        if (data->state == SYN_RECEIVED) {
            auto now = chrono::steady_clock::now();
            if (data->socket.getLastError() == 0) {
                if (now - data->lastTimeSentNewData < SYN_ACK_DELAY) return;
                packet.setSourceAddress(data->serverAddress);
                packet.setDestinationAddress(data->clientAddress);
                packet.makeSyn(data->sendUnacknowledged, data->receiveSent);
                sendBack(packet);
                data->lastTimeSentNewData = chrono::steady_clock::now();
                data->state = SYN_ACK_SENT;
                data->socket.unsetFrom(data->sendSet);
            } else {
                packet.setSourceAddress(data->serverAddress);
                packet.setDestinationAddress(data->clientAddress);
                packet.makeResetAck(data->receiveSent);
                sendBack(packet);
#ifdef LOGGING
                ::printf("Sent ack from 'flushDataToServer at SYN_RECEIVED state'\n");
#endif
                closeSession();
            }
            return;
        }
        if (data->state != ESTABLISHED || data->flushToServerFinished) return;

        auto total = data->receiveBuffer.position();
        auto amt = data->receiveBuffer.manipulate([&](char *buf, unsigned int len) -> unsigned int {
            try {
                auto res = data->socket.sendIgnoreWouldBlock(buf, (int) len);
                if (res == -1)return 0;
                return res;
            }
            catch (SocketError &e) {
#ifdef LOGGING
                cout << "Server's send exited with an error: \n   " << e.what() << endl;
#endif
                packet.makeResetSeq(data->sendUnacknowledged + data->sendNextOffset);
                packet.setSourceAddress(data->serverAddress);
                packet.setDestinationAddress(data->clientAddress);
                sendBack(packet);
                closeSession();
                return 0;
            }
        }, 0, total);

        if (data->state != ESTABLISHED) return;
        if (amt == total) {
            data->socket.unsetFrom(data->sendSet);
            if (data->receiveFromClientFinished) {
                if (data->flushToClientFinished) closeSession();
                else {
                    data->socket.shutdownWrite();
                    data->flushToServerFinished = true;
                }
                return;
            }
        }
        data->receiveBuffer.compact(total - amt);
    }

    void flushDataToClient(CompressedTCPPacket &packet, function<void(CompressedTCPPacket &)> sendBack) {
        if (data->state != ESTABLISHED || data->flushToClientFinished) return;

        const auto now = chrono::steady_clock::now();
        const auto bound = min(data->sendWindowRightEdge, data->sendBuffer.position());
        const bool finNotSent = data->receiveFromClientFinished != data->sentFinToClient;
        const bool finNotAcknowledged = data->receiveFromServerFinished != data->flushToClientFinished;

        auto sendDataStartingFrom = [&](unsigned int off, unsigned short mss, bool push = false) {
            auto ack = data->receiveSent + data->receiveBuffer.position();
            packet.setWindow(data->receiveBuffer.available());
            while ((push ? off : off + mss) < bound + data->receiveFromServerFinished) {
                auto amt = min((unsigned int) mss, bound - off);
                packet.makeNormal(data->sendUnacknowledged + off, ack);
                packet.setTCPPayload(data->sendBuffer.as(off, off + amt));
                off += amt;
                if (data->receiveFromServerFinished &&
                    off == data->sendBuffer.position()) {
                    packet.setFinFlag(true);
                    data->sentFinToClient = true;
                }
                sendBack(packet);
            }
            auto newOff = max(data->sendNextOffset, off);
            if (newOff != data->sendNextOffset) {
                data->sendNextOffset = newOff;
                data->lastTimeSentNewData = now;
                data->sendRetryCount = 0;
            }
        };

        if (data->sendNextOffset || finNotAcknowledged &&
                                    now - data->lastTimeAcceptedAcknowledgement > data->getMeanRTT() / 5) {
            sendDataStartingFrom(0, data->mss, finNotAcknowledged);
        } else if (bound - data->sendNextOffset > data->mss) {
            sendDataStartingFrom(data->sendNextOffset, data->mss);
            if (!data->rttTracking) {
                data->rttTracking = true;
                data->rttTrackedSendSequence = data->sendUnacknowledged + data->sendNextOffset + data->sentFinToClient;
                data->rttSendTime = now;
            }
        }

        /*should flush first, before checking for probing*/
        if (data->sendNextOffset != bound && now - data->lastTimeSentNewData > data->getMeanRTT() / 5 || finNotSent) {
            sendDataStartingFrom(data->sendNextOffset, data->mss, true);
        }

        if (now - data->lastTimeSentNewData > getIntervalToNextProbe()) {
            data->sendRetryCount++;
            packet.makeNormal(data->sendUnacknowledged + data->sendNextOffset,
                              data->receiveSent + data->receiveBuffer.position());
            packet.setWindow(data->receiveBuffer.available());
            sendBack(packet);
        }
    }

    void onSocketError(CompressedTCPPacket &packet, function<void(CompressedTCPPacket &)> sendBack) {
#ifdef LOGGING
        cout << "onSocketError was called" << endl;
#endif
        packet.makeResetSeq(data->sendUnacknowledged + data->sendNextOffset);
        packet.setSourceAddress(data->serverAddress);
        packet.setDestinationAddress(data->clientAddress);
        sendBack(packet);
        closeSession();
    }

    bool checkStatus() {
        switch (data->state) {
            case CLOSED:
                return false;
            case SYN_ACK_SENT:
                if (chrono::steady_clock::now() - data->lastTimeSentNewData > SYN_TIMEOUT) {
                    closeSession();
                    return false;
                }
            case ESTABLISHED:
                if (chrono::steady_clock::now() - data->lastTimeAcceptedAcknowledgement >
                    (data->flushToClientFinished ? NO_PROBING_TIMEOUT : getIntervalToNextProbe() +
                                                                        data->getMeanRTT() * 2)) {
                    closeSession();
                    return false;
                }
            default:
                break;
        }
        return true;
    }

    TCPSocket &getSessionSocket() {
        return data->socket;
    }

    void setOnSocketClose(function<void()> callback) {
        if (!callback)throw invalid_argument("Empty callback is disallowed");
        data->onSocketClose = callback;
    }

    function<void()> &getOnSocketClose() {
        return data->onSocketClose;
    }

    void closeSession() {
        data->socket.unsetFrom(data->receiveSet);
        data->socket.unsetFrom(data->sendSet);
        data->socket.unsetFrom(data->errorSet);
        data->state = CLOSED;
        data->socket.close();
        data->onSocketClose();
    }

private:

    constexpr static unsigned int BUFFER_SIZE = numeric_limits<unsigned short>::max();

    constexpr static chrono::milliseconds ACKNOWLEDGE_DELAY{100};

    struct Data {
        fd_set &receiveSet;
        fd_set &sendSet;
        fd_set &errorSet;
        function<void()> onSocketClose{[] {}};
        TCPSocket socket{DEFER_INIT};

        states state{CLOSED};
        bool receiveFromClientFinished{};
        bool flushToServerFinished{};
        bool receiveFromServerFinished{};
        bool sentFinToClient{};
        bool flushToClientFinished{};

        sockaddr_storage clientAddress{};
        sockaddr_storage serverAddress{};

        SmartBuffer<char> sendBuffer{BUFFER_SIZE};
        unsigned int sendWindowRightEdge{};
        unsigned char sendWindowShift{};
        unsigned int sendUnacknowledged{};
        unsigned int sendNextOffset{};
        unsigned int sendRetryCount{};

        SmartBuffer<char> receiveBuffer{BUFFER_SIZE};
        unsigned int receiveSent{};

        unsigned short mss{IP_MSS};
        vector<chrono::milliseconds> rttHistory{5, MIN_RTT};
        bool rttTracking{};
        unsigned int rttTrackedSendSequence{};
        chrono::steady_clock::time_point rttSendTime{};
        chrono::steady_clock::time_point lastTimeSentNewData{};
        chrono::steady_clock::time_point lastTimeAcceptedAcknowledgement{};

        Data(fd_set &rcv, fd_set &snd, fd_set &err) : receiveSet(rcv), sendSet(snd),
                                                      errorSet(err) {
            for (auto &rtt: rttHistory) {
                rtt = MIN_RTT * 5;
            }
        }

        void reset() {
            state = CLOSED; //todo : might remove
            receiveFromClientFinished = flushToServerFinished = receiveFromServerFinished = sentFinToClient = flushToClientFinished = false;

            sendBuffer.clear();
            sendUnacknowledged = generateSequenceNo();
            sendNextOffset = sendWindowRightEdge = 0;
            sendWindowShift = 0; //todo : might remove
            sendRetryCount = 0;

            receiveBuffer.clear();
            receiveSent = 0; //todo : might remove

            mss = IP_MSS;
            for (auto &rtt: rttHistory) {
                rtt = MIN_RTT * 5;
            }
            rttTracking = false;
            lastTimeSentNewData = {};
        }

        inline void pushRTT(const chrono::milliseconds &&rtt) {
            pushRTT(rtt);
        }

        inline void pushRTT(const chrono::milliseconds &rtt) {
            rttHistory.erase(rttHistory.cbegin());
            rttHistory.push_back(max(rtt, MIN_RTT));
        }

        inline chrono::milliseconds getMeanRTT() {
            chrono::milliseconds result{};
            for (auto &rtt: rttHistory) {
                result += rtt;
            }
            return result / rttHistory.size();
        }


    };

    shared_ptr<Data> data;

    inline static unsigned int generateSequenceNo() {
        static uniform_int_distribution dist{static_cast <unsigned int>(0), numeric_limits<unsigned int>::max()};
        static default_random_engine engine{
                static_cast<unsigned int>(chrono::steady_clock::now().time_since_epoch().count())};
        return dist(engine);
    }

    void acknowledgeDelayed(CompressedTCPPacket &packet, const function<void(CompressedTCPPacket &)> &sendBack) {
        auto now = chrono::steady_clock::now();
        static chrono::steady_clock::time_point lastTimeSentAcknowledge{};
        if (now - lastTimeSentAcknowledge > ACKNOWLEDGE_DELAY) {
            packet.makeNormal(data->sendUnacknowledged + data->sendNextOffset,
                              data->receiveSent + data->receiveBuffer.position());
            packet.setSourceAddress(data->serverAddress);
            packet.setDestinationAddress(data->clientAddress);
            sendBack(packet);
            lastTimeSentAcknowledge = now;
        }
    }

    [[nodiscard]] inline chrono::milliseconds getIntervalToNextProbe() const {
        return ((data->sendRetryCount + 1) * data->getMeanRTT());
    }
};


#endif //TUNSERVER_TCPSESSION_H

#pragma clang diagnostic pop