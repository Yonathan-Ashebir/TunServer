//
// Created by yoni_ash on 4/30/23.
//
/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSE closesocket
#define socket_t unsigned long
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#define socket_t int
#define CLOSE close
#endif



#define MYPORT "4950"    // the port users will be connecting to
#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int main(void) {
    socket_t sockFd;
    struct addrinfo hints, *servInfo, *p;
    int rv;
    size_t numBytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servInfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servInfo; p != NULL; p = p->ai_next) {
        if ((sockFd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockFd, p->ai_addr, p->ai_addrlen) == -1) {
            CLOSE(sockFd);
            perror("listener: bind");
            continue;
        }
        printf("listener: bound to %s:%d\n",
               inet_ntop(p->ai_family,
                         get_in_addr((struct sockaddr *) &p->ai_addr),
                         s, sizeof s), ntohs(((struct sockaddr_in *) &(p->ai_addr))->sin_port));

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servInfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numBytes = recvfrom(sockFd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("listener: got packet from %s\n",
           inet_ntop(their_addr.ss_family,
                     get_in_addr((struct sockaddr *) &their_addr),
                     s, sizeof s));
    printf("listener: packet is %zu bytes long\n", numBytes);
    buf[numBytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    CLOSE(sockFd);

    return 0;
}