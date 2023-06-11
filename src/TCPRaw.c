//
// Created by yoni_ash on 5/1/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char ihl: 4;
    unsigned char version: 4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error        "Please fix <bits/endian.h>"
#endif
    unsigned char tos;
    unsigned short tot_len;
    unsigned short id;
    unsigned short frag_off;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short check;
    unsigned int saddr;
    unsigned int daddr;
    /*The options start here. */
};

struct tcphdr {
    unsigned short source;
    unsigned short dest;
    unsigned int seq;
    unsigned int ack_seq;
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned short res1: 4;
    unsigned short doff: 4;
    unsigned short fin: 1;
    unsigned short syn: 1;
    unsigned short rst: 1;
    unsigned short psh: 1;
    unsigned short ack: 1;
    unsigned short urg: 1;
    unsigned short res2: 2;
#  elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned short doff:4;
    unsigned short res1:4;
    unsigned short res2:2;
    unsigned short urg:1;
    unsigned short ack:1;
    unsigned short psh:1;
    unsigned short rst:1;
    unsigned short syn:1;
    unsigned short fin:1;
#  else
#   error "Adjust your <bits/endian.h> defines"
#  endif
    unsigned short window;
    unsigned short check;
    unsigned short urg_ptr;
};


// pseudo header needed for tcp header checksum calculation
struct pseudo_header {
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;
};

#define DATAGRAM_LEN 4096
#define OPT_SIZE 20

unsigned short checksum(const char *buf, unsigned size) {
    unsigned sum = 0, i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        unsigned short word16 = *(unsigned short *) &buf[i];
        sum += word16;
    }

    /* Handle odd-sized case */
    if (size & 1) {
        unsigned short word16 = (unsigned char) buf[i];
        sum += word16;
    }

    /* Fold to get the ones-complement result */
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

    /* Invert to get the negative in ones-complement arithmetic */
    return ~sum;
}

void create_syn_packet(struct sockaddr_in *src, struct sockaddr_in *dst, char **out_packet, int *out_packet_len) {
    // datagram to represent the packet
    char *datagram = calloc(DATAGRAM_LEN, sizeof(char));

    // required structs for IP and TCP header
    struct iphdr *iph = (struct iphdr *) datagram;
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof(struct iphdr));
    struct pseudo_header psh;

    // IP header configuration
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
    iph->id = htonl(rand() % 65535); // id of this packet
    iph->frag_off = 0; // 1<<6 == do not frag
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; // correct calculation follows later
    iph->saddr = src->sin_addr.s_addr;
    iph->daddr = dst->sin_addr.s_addr;

    // TCP header configuration
    tcph->source = src->sin_port;
    tcph->dest = dst->sin_port;
    tcph->seq = htonl(rand() % 4294967295);
    tcph->ack_seq = htonl(0);
    tcph->doff = 10; // tcp header size
    tcph->fin = 0;
    tcph->syn = 1;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 0;
    tcph->urg = 0;
    tcph->check = 0; // correct calculation follows later
    tcph->window = htons(5840); // window size: large one seen = 64240
    tcph->urg_ptr = 0;

    // TCP pseudo header for checksum calculation
    psh.source_address = src->sin_addr.s_addr;
    psh.dest_address = dst->sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE;
    // fill pseudo packet
    char *pseudogram = malloc(psize);
    memcpy(pseudogram, (char *) &psh, sizeof(struct pseudo_header));

    // TCP options are only set in the SYN packet
    int optionsOffset = sizeof(struct iphdr) + sizeof(struct tcphdr);
    // ---- set mss ----
    datagram[optionsOffset] = 0x02;
    datagram[optionsOffset + 1] = 0x04;
    unsigned short mss = htons(1460); // mss value
    memcpy(datagram + optionsOffset + 2, &mss, sizeof(mss));
    // ---- enable SACK ----
    datagram[optionsOffset + 4] = 0x04;
    datagram[optionsOffset + 5] = 0x02;

    /*    //Time stamp
    datagram[optionsOffset + 6] = 8;
    datagram[optionsOffset + 7] = 10;
    int t = htonl(time(NULL));
    int t2 = htonl(0);
    memcpy(datagram + optionsOffset + 8, &t, sizeof t);
    memcpy(datagram + optionsOffset + 12, &t2, sizeof t);
    //No-Operation
    datagram[optionsOffset + 16] = 1;
    //Window scale
    datagram[optionsOffset + 17] = 3;
    datagram[optionsOffset + 18] = 3;
    datagram[optionsOffset + 19] = 7;*/

    // do the same for the pseudo header
    memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + OPT_SIZE);


    tcph->check = checksum((const char *) pseudogram, psize);
    iph->check = checksum((const char *) datagram, iph->tot_len);

    *out_packet = datagram;
    *out_packet_len = iph->tot_len;
    free(pseudogram);
}

void
create_ack_packet(struct sockaddr_in *src, struct sockaddr_in *dst, int seq, int ack_seq, char **out_packet,
                  int *out_packet_len) {
    // datagram to represent the packet
    char *datagram = calloc(DATAGRAM_LEN, sizeof(char));

    // required structs for IP and TCP header
    struct iphdr *iph = (struct iphdr *) datagram;
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof(struct iphdr));
    struct pseudo_header psh;

    // IP header configuration
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
    iph->id = htonl(rand() % 65535); // id of this packet
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; // correct calculation follows later
    iph->saddr = src->sin_addr.s_addr;
    iph->daddr = dst->sin_addr.s_addr;

    // TCP header configuration
    tcph->source = src->sin_port;
    tcph->dest = dst->sin_port;
    tcph->seq = htonl(seq);
    tcph->ack_seq = htonl(ack_seq);
    tcph->doff = 10; // tcp header size
    tcph->fin = 0;
    tcph->syn = 0;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 1;
    tcph->urg = 0;
    tcph->check = 0; // correct calculation follows later
    tcph->window = htons(5840); // window size
    tcph->urg_ptr = 0;

    // TCP pseudo header for checksum calculation
    psh.source_address = src->sin_addr.s_addr;
    psh.dest_address = dst->sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE;
    // fill pseudo packet
    char *pseudogram = malloc(psize);
    memcpy(pseudogram, (char *) &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + OPT_SIZE);

    tcph->check = checksum((const char *) pseudogram, psize);
    iph->check = checksum((const char *) datagram, iph->tot_len);

    *out_packet = datagram;
    *out_packet_len = iph->tot_len;
    free(pseudogram);
}

void create_data_packet(struct sockaddr_in *src, struct sockaddr_in *dst, int seq, int ack_seq, char *data,
                        int data_len, char **out_packet, int *out_packet_len) {
    // datagram to represent the packet
    char *datagram = calloc(DATAGRAM_LEN, sizeof(char));

    // required structs for IP and TCP header
    struct iphdr *iph = (struct iphdr *) datagram;
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof(struct iphdr));
    struct pseudo_header psh;

    // set payload
    char *payload = datagram + sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
    memcpy(payload, data, data_len);

    // IP header configuration
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE + data_len;
    iph->id = htonl(rand() % 65535); // id of this packet
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; // correct calculation follows later
    iph->saddr = src->sin_addr.s_addr;
    iph->daddr = dst->sin_addr.s_addr;

    // TCP header configuration
    tcph->source = src->sin_port;
    tcph->dest = dst->sin_port;
    tcph->seq = htonl(seq);
    tcph->ack_seq = htonl(ack_seq);
    tcph->doff = 10; // tcp header size
    tcph->fin = 0;
    tcph->syn = 0;
    tcph->rst = 0;
    tcph->psh = 1;
    tcph->ack = 1;
    tcph->urg = 0;
    tcph->check = 0; // correct calculation follows later
    tcph->window = htons(5840); // window size
    tcph->urg_ptr = 0;

    // TCP pseudo header for checksum calculation
    psh.source_address = src->sin_addr.s_addr;
    psh.dest_address = dst->sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE + data_len);
    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE + data_len;
    // fill pseudo packet
    char *pseudogram = malloc(psize);
    memcpy(pseudogram, (char *) &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + OPT_SIZE + data_len);

    tcph->check = checksum((const char *) pseudogram, psize);
    iph->check = checksum((const char *) datagram, iph->tot_len);

    *out_packet = datagram;
    *out_packet_len = iph->tot_len;
    free(pseudogram);
}

void read_seq_and_ack(const char *packet, unsigned int *seq, unsigned int *ack) {
    // read sequence number
    unsigned int seq_num;
    memcpy(&seq_num, packet + 24, 4);
    // read acknowledgement number
    unsigned int ack_num;
    memcpy(&ack_num, packet + 28, 4);
    // convert network to host byte order
    *seq = ntohl(seq_num);
    *ack = ntohl(ack_num);
    printf("sequence number: %lu\n", (unsigned long) *seq);
    printf("acknowledgement number: %lu\n", (unsigned long) *seq);
}

int receive_from(int sock, char *buffer, size_t buffer_length, struct sockaddr_in *dst) {
    unsigned short dst_port;
    int received;
    do {
        received = recvfrom(sock, buffer, buffer_length, 0, NULL, NULL);
        if (received == -1)
            break;
        memcpy(&dst_port, buffer + 22, sizeof(dst_port));
    } while (dst_port != dst->sin_port);
    printf("received bytes: %d\n", received);
    printf("destination port: %d\n", ntohs(dst->sin_port));
    return received;
}

int main(int argc, char **argv) {
//    if (argc != 4) {
//        printf("invalid parameters.\n");
//        printf("USAGE %s <source-ip> <target-ip> <port>\n", argv[0]);
//        return 1;
//    }

    srand(time(NULL));

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock == -1) {
        printf("socket creation failed\n");
        return 1;
    }

    // destination IP address configuration
    struct sockaddr_in daddr;
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(atoi(argv[3]));
    if (inet_pton(AF_INET, argv[2], &daddr.sin_addr) != 1) {
        printf("destination IP configuration failed\n");
        return 1;
    }

    // source IP address configuration
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(3333); // random client port
    if (inet_pton(AF_INET, argv[1], &saddr.sin_addr) != 1) {
        printf("source IP configuration failed\n");
        return 1;
    }

    // call bind with port number specified as zero to get an unused source port
    if (bind(sock, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) == -1) {
        printf("bind() failed\n");
        return 1;
    }

//     // retrieve source port
//     socklen_t addrLen = sizeof(struct sockaddr);
//     if (getsockname(sock, (struct sockaddr*)&saddr, &addrLen) == -1)
//     {
//     	printf("getsockname() failed\n");
//     	return 1;
//     }
    printf("selected source port number: %d\n", ntohs(saddr.sin_port));

    // tell the kernel that headers are included in the packet
#ifdef _WIN32
    char one = 1;
#else
    int one = 1;
#endif
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) == -1) {
        printf("setsockopt(IP_HDRINCL, 1) failed\n");
        return 1;
    }

    // send SYN
    char *packet;
    int packet_len;
    create_syn_packet(&saddr, &daddr, &packet, &packet_len);

    int sent;
    if ((sent = sendto(sock, packet, packet_len, 0, (struct sockaddr *) &daddr, sizeof(struct sockaddr))) == -1) {
        printf("sendto() failed\n");
    } else {
        printf("successfully sent %d bytes SYN!\n", sent);
    }

    // receive SYN-ACK
    char recvbuf[DATAGRAM_LEN];
    int received = receive_from(sock, recvbuf, sizeof(recvbuf), &saddr);
    if (received <= 0) {
        printf("receive_from() failed\n");
    } else {
        printf("successfully received %d bytes SYN-ACK!\n", received);
    }

    // read sequence number to acknowledge in next packet
    unsigned int seq_num, ack_num;
    read_seq_and_ack(recvbuf, &seq_num, &ack_num);
    int new_seq_num = seq_num + 1;

    // send ACK
    // previous seq number is used as ack number and vica vera
    create_ack_packet(&saddr, &daddr, ack_num, new_seq_num, &packet, &packet_len);
    if ((sent = sendto(sock, packet, packet_len, 0, (struct sockaddr *) &daddr, sizeof(struct sockaddr))) == -1) {
        printf("sendto() failed\n");
    } else {
        printf("successfully sent %d bytes ACK!\n", sent);
    }

    // send data
    char request[] = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    create_data_packet(&saddr, &daddr, ack_num, new_seq_num, request, sizeof(request) - 1 / sizeof(char), &packet,
                       &packet_len);
    if ((sent = sendto(sock, packet, packet_len, 0, (struct sockaddr *) &daddr, sizeof(struct sockaddr))) == -1) {
        printf("send failed\n");
    } else {
        printf("successfully sent %d bytes PSH!\n", sent);
    }

    // receive response
    while ((received = receive_from(sock, recvbuf, sizeof(recvbuf), &saddr)) > 0) {
        printf("successfully received %d bytes!\n", received);
        read_seq_and_ack(recvbuf, &seq_num, &ack_num);
        new_seq_num = seq_num + 1;
        create_ack_packet(&saddr, &daddr, ack_num, new_seq_num, &packet, &packet_len);
        if ((sent = sendto(sock, packet, packet_len, 0, (struct sockaddr *) &daddr, sizeof(struct sockaddr))) == -1) {
            printf("send failed\n");
        } else {
            printf("successfully sent %d bytes ACK!\n", sent);
        }
    }

    // TODO: handle FIN packets to close the session properly

    CLOSE(sock);
    return 0;
}