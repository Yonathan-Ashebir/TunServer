//
// Created by yoni_ash on 4/21/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_PACKET_H
#define TUNSERVER_PACKET_H


#endif //TUNSERVER_PACKET_H

#include "../Include.h"
#include "Packet.h"

class TCPPacket : public Packet {
public:

    explicit TCPPacket(unsigned int size);

    inline void setEnds(sockaddr_in &src, sockaddr_in &dest);

    inline void setSource(sockaddr_in &src);

    inline void setDestination(sockaddr_in &dest);


    inline void setSequenceNumber(unsigned int seq);

    inline void setAcknowledgmentNumber(unsigned int ack);

    inline void setUrgentPointer(unsigned short urg);

    inline void setSynFlag(bool isSyn);

//   inline void setAckFlag(bool isSyn);

//    inline void setUrgentFlag(bool isUrg);

    inline void setFinFlag(bool isFin);

    inline void setResetFlag(bool isRst);

    inline void setPushFlag(bool isPush);

    inline void setWindowSize(unsigned short size);

    inline void setWindowScale(unsigned char shift);

    inline void setWindowSize(unsigned short window, unsigned char shift);

    inline void setMSS(unsigned short size);

    void setOption(unsigned char kind, unsigned char len, unsigned char *payload);

    bool removeOption(unsigned char kind);

    inline sockaddr_in getSource();

    inline sockaddr_in getDestination();

    inline bool isFrom(sockaddr_in &src);

    inline bool isTo(sockaddr_in &dest);

    inline unsigned int getSequenceNumber();

    inline unsigned int getAcknowledgmentNumber();

    inline unsigned short getUrgentPointer();

    inline bool isSyn();

    inline bool isFin();

    inline bool isReset();

    inline bool isPush();

    inline bool isAck();

    inline bool isUrg();

    inline unsigned short getWindowSize();

    inline unsigned char getWindowShift();

    inline unsigned short getMSS();

    unsigned char getOption(unsigned char kind, unsigned char *data, unsigned char len);

    inline unsigned int appendData(unsigned char *data, unsigned int len);

    inline void clearData();

    inline unsigned int getDataLength();

    inline unsigned int available() const;

    inline unsigned int copyDataTo(unsigned char *buff, unsigned int len);


    inline void makeSyn(unsigned int seq, unsigned int ack);

    inline void makeResetSeq(unsigned int seq);

    inline void makeResetAck(unsigned int ack);

    inline void makeFin(unsigned int seq, unsigned int ack);

    void makeNormal(unsigned int seqNo, unsigned int ackSeq);

    inline void swapEnds();

    unsigned int getSegmentLength();

private:

    unsigned int optionsLength = 0;


    inline tcphdr *getTcpHeader();

};

#pragma clang diagnostic pop