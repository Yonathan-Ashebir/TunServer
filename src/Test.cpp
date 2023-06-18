//
// Created by yoni_ash on 4/15/23.
//
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "Include.h"
#include "./packet/TCPPacket.h"
#include "./tunnel/DatagramTunnel.h"

using namespace std;

#define ipAddr "127.0.0.1"

void printSizeC() {
    printf("Char size: %llu\n", sizeof(char));
}


void sizeOfArrayAndElement() {
    char arr[30];
    typeof(arr + 1) el;
    printf("Arr size: %lu and its element: %lu\n", sizeof(arr), sizeof(el));
}

struct __attribute__((unused)) boolArray {
    bool a;
    bool b;
    bool c;
};

void shiftBy(int arr[], int ind, int len, int by) {
    memmove(arr + ind + by, arr + ind, len);
}

void shiftTest() {
    int arr[] = {1, 2, 3, 4, 5};
    shiftBy(arr, 2, 1, -1);
    printf("Result: %d,%d,%d,%d,%d", arr[0], arr[1], arr[2], arr[3], arr[4]);
}

sockaddr_in _selectTestBindAddress;

void selectTest() {
    int listener = createTcpSocket();
    _selectTestBindAddress = {AF_INET, 3333};
    if (bind(listener, (sockaddr *) &_selectTestBindAddress, sizeof(sockaddr_in)) == -1)exitWithError("Could not bind");
    listen(listener, 20);


    pthread_t sendThread;


    pthread_create(&sendThread, nullptr, [](void *ptr) -> void * {
        int sender = createTcpSocket();
        if (connect(sender, (sockaddr *) &_selectTestBindAddress, sizeof(sockaddr_in)) == -1)
            printError("Could not connect");
        const char message[] = "How long the country will last is uncertain"
                               "How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain";

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
        while (true) {
            size_t sent = send(sender, message, sizeof message, 0);
            if (isWouldBlock()) {
//                cout << "Would"
            } else if (errno != 0) {
                break;
            }
        }
#pragma clang diagnostic pop
    }, nullptr);
}

void mathRoofTest() {
    cout << (6 / 4 * 4) << endl;
}

void stringCatTest() {
    cout << ("hello " "world") << endl;
    cout << ("hello " + to_string(2)) << endl;
    char world[] = {'w', 'o', 'r', 'l', 'd'};
//    cout << ("hello " + world) << endl;
}

void sizeAfterCastTest() {
    unsigned int a = 1;
    unsigned short s = UINT16_MAX - 1 + a;
    unsigned int b = s - a;
    cout << "Before: " << s << endl;
    cout << "Without casting: " << (s << 1) << endl;
    cout << "After: " << ((int) s << 1) << endl;
    cout << "a: " << a << endl;
    cout << "b: " << b << endl;
}

struct BitStruct {
    unsigned int a: 1;
    unsigned int b: 7;
};

void overflowAssignmentTest() {
    BitStruct bits;
    bits.a = 2; //understand from the warning !!!
    unsigned short s = 65536;// do the same
}

class Cyver {
    short cyver;

};

void textCLassSize() {
    cout << sizeof(Cyver) << endl;
}

void testUniformInit() {
    cout << "With no init" << endl;
    int arr[10];
    int i;
    sockaddr_in addr;

    printf("arr[9]: %d and arr[7]: %d\n", arr[9], arr[7]);
    printf("i: %d\n", i);
    printf("addr.source: %d and port: %d\n", addr.sin_addr.s_addr, addr.sin_port);

    cout << "With uniform init" << endl;

    int arr2[10]{};
    int i2{};
    sockaddr_in addr2{};

    printf("arr[9]: %d and arr[7]: %d\n", arr2[9], arr2[7]);
    printf("i: %d\n", i2);
    printf("addr.source: %d and port: %d\n", addr2.sin_addr.s_addr, addr2.sin_port);

}

void measureCPUTime(void call()) {
    ::clock_t start, end;
    start = ::clock();
    call();
    end = ::clock();
    long diff = end - start;
    printf("Spent %ld clocks or %ld millis\n", diff, diff / CLOCKS_PER_SEC * 1000);
}

void measureElapsedTime(void call()) {
    auto start_time = std::chrono::high_resolution_clock::now();
    call();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    printf("Spent %ld millis\n", time / std::chrono::milliseconds(1));

}

sockaddr_in measureConnectTimeAddr;

void measureConnectTime() {
    auto call = [] {
        addrinfo *info;

        int result = getaddrinfo("www.google.com", "80", nullptr, &info);
        if (result != 0) {
            perror(gai_strerror(result));
            exit(1);
        }
        char buf[INET_ADDRSTRLEN];

        inet_ntop(info->ai_family, &((sockaddr_in *) info->ai_addr)->sin_addr, buf, sizeof buf);
        ::printf("Ip addr: %s\n", buf);
        measureConnectTimeAddr = *(sockaddr_in *) info->ai_addr;
    };
    measureElapsedTime(call);
//    measureConnectTimeAddr.sin_port = htons(55557);
    char c;
    cin >> c;
    measureElapsedTime([] {
        int sock = createTcpSocket();
        while (true) {
            if (connect(sock, (sockaddr *) &measureConnectTimeAddr, sizeof measureConnectTimeAddr) == -1) {
                printError("Could not connect");
                socklen_t len;
                int err = 0;


                int res = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
                if (res != 0)exitWithError("Failed to get socket options");
                printError("Socket option error", err);

                usleep(10000);
            } else {
                ::printf("Connected Successfully\n");
                int lastTest = connect(sock, (sockaddr *) &measureConnectTimeAddr, sizeof measureConnectTimeAddr);
                if (lastTest != 0)printError("Error after success");
                break;
            }

        }
    });
}

void testSocketIO() {
    sockaddr_in addr{};
    inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(80);
    addr.sin_family = AF_INET;

    int sock = createTcpSocket();
    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1)exitWithError("Could not connect");
    errno = 0;
    sleep(1);
    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1) {
        exitWithError("Could not connect");
    } else {
        ::printf("Connected Successfully\n");
    }
    const char *buf = "GET / HTTP/1.0\r\n\r\n";
    size_t total = send(sock, buf, 0, 0);
    printError("After send");
    cout << "Total: " << total << endl;
    errno = 0;


    total = send(sock, buf, sizeof(buf), 0);
    printError("After send");
    ::printf("Total: %zu\n", total);
    errno = 0;
    cout << endl;

}

void testRawTypeSize() {
    printf("Size of char: %lu\n", sizeof(char));
    printf("Size of short: %lu\n", sizeof(short));
    printf("Size of int: %lu\n", sizeof(int));
    printf("Size of long int: %lu\n", sizeof(long int));
    printf("Size of long: %lu\n", sizeof(long));
    printf("Size of long long: %lu\n", sizeof(long long));
    printf("Size of float: %lu\n", sizeof(float));
    printf("Size of double: %lu\n", sizeof(double));
    printf("Size of long double: %lu\n", sizeof(long double));
}

//class Friend {
//public:
//   void fun();
//};
//
//class Alice {
//void Friend::fun() {};
//};

enum class testEnum {
    enumA, enumB
};

/*This blocks manages to deactivate CLION's intellisense demanding restart to fix it.*/
//void testSelfIncludingClass() {
//    class Recur {
//    public:
//        Recur() {
//            printf("Recur constructor called\n");
//        }
//
//        int a{};
////        Recur r1;
//        Recur *r2 = new Recur;
//    };
//    Recur recur;
//};

void UDPSocketTest() {
    int sock1 = createUdpSocket();
    int sock2 = createUdpSocket();
    if (sock1 == -1 || sock2 < 2)exitWithError("Could not create sockets");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.4", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3334);
    auto b2 = bind(sock2, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b2 == -1)exitWithError("Could not bind sock2");

    addr.sin_port = htons(3333);
    auto b1 = bind(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b1 == -1)exitWithError("Could not bind sock1");

//    int l = listen(sock1, 20);
    thread t{
            [&] {
                ::printf("Waiting at 3333\n");
                char buf[3072];
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
                while (true) {
                    sockaddr_in from{};
                    socklen_t len;
                    auto r = recvfrom(sock1, buf, sizeof buf, 0, reinterpret_cast<sockaddr *>(&from),
                                      &len);

                    if (r > 0) {
                        char ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
                        printf("Received %zd bytes from %s:%d\n", r, ip, ntohs(from.sin_port));
                    } else
                        exitWithError("Could not receive");
                }
#pragma clang diagnostic pop
            }
    };


    while (true) {
        char buf[200];
        scanf("%s", buf);
        errno = 0;
        size_t s = sendto(sock2, buf, strlen(buf) + 1, 0, (sockaddr *) &addr, sizeof addr);
        if (errno != 0)exitWithError("Could not send the message");
    }

}

void tunClientTest() {
    socket_t sock = createUdpSocket();
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);

    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1)exitWithError("Could not connect");
    char msg[] = "HELLO";
    if (send(sock, msg, (int) ::strlen(msg), 0) < strlen(msg))exitWithError("Could not send");

}

void testVpnSubscriber() {
    socket_t sock = createUdpSocket();
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ipAddr, &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);
    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1) {//warn: this might not be appopirate for windows.
#ifdef _WIN32
        printError("Could not connect", WSAGetLastError());
        exit(1);
#else
        exitWithError("Could not connect");

#endif
    }

    DatagramTunnel tunnel(sock);

    TCPPacket packet(3072);
    addr.sin_port = 55555;
    packet.setSource(addr);
    addr.sin_port = 3335;
    packet.setDestination(addr);
    packet.makeSyn(1, 0);
    tunnel.writePacket(packet);

}

[[noreturn]] void udpFlushTest() {
    int sock = createUdpSocket();
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(80);

    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1)exitWithError("Could not connect");
    char msg[5000]{'H'};
    auto msgStr = string(5000, 'H');

    TCPPacket packet(3072);
    DatagramTunnel tunnel(sock);
    while (true) {
#define msg_udp_flush msgStr.c_str()
        if (send(sock, msg_udp_flush, ::strlen(msg_udp_flush), 0) < strlen(msg_udp_flush))
            exitWithError("Could not send");

//        packet.makeNormal(100, 100);
//        packet.setWindowSize(65535);
//        packet.clearData();
//        packet.appendData(reinterpret_cast<unsigned char *>(msg), 516);
//        tunnel.writePacket(packet);

        /*Memory allocation with flush test*/
/*        char buf[5000];
        char buf2[5000];
//        char *buf = new char[5000];
//        char *buf2 = new char[5000];
        memcpy(buf, buf2, 5000);
//        delete[] buf;
//        delete[] buf2;*/

        /*Arithmetic with flushing*/
        /* unsigned int count = 0;
         while (count < 1000) {
             count++;
             rand();
         }*/

        /*Packet management*/


    }
}

void memoryAllocateTest() {
    while (true) {
        char *buf = new char[5000];
        delete[] buf;
    }
}

void startHelloServer() {
    auto sock = createTcpSocket();
    if (sock == -1)exitWithError("Could not create a socket");

    sockaddr_in address{};
    socklen_t len;
    inet_pton(AF_INET, "192.168.1.4", &address.sin_addr);
    address.sin_port = htons(3335);
    address.sin_family = AF_INET;

    if (bind(sock, (sockaddr *) &address, sizeof address) == -1)exitWithError("Could not bind");
    if (listen(sock, 10) == -1)exitWithError("Could not listen");

    char buf[INET_ADDRSTRLEN];
    memset(buf, 0, INET_ADDRSTRLEN);

    if (inet_ntop(AF_INET, &address.sin_addr, buf, INET_ADDRSTRLEN) == NULL) {
        exitWithError("Could not log server:host info");
    }

    printf("Listening at %s:%d\n", buf, ntohs(address.sin_port));
    while (true) {
        sockaddr_in addr{};
        len = sizeof addr;
        auto client = accept(sock, (sockaddr *) &addr, &len);
        if (client == -1) {
            exitWithError("Could not accept a client");
        }

        memset(buf, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
            exitWithError("Could not log server:host info");
        printf("Accepted client from %s:%d\n", buf, ntohs(addr.sin_port));

        auto message = "Hello World!\n";

        send(client, message, strlen(message), 0);//assuming it sends it all at ounce
//        sleep(1000);

        CLOSE(client);
    }
}

[[noreturn]] void startHelloAndListen() {
    auto sock = createTcpSocket(true);

    sockaddr_in address{};
    socklen_t len;
    inet_pton(AF_INET, "0.0.0.0", &address.sin_addr);
    address.sin_port = htons(3333);
    address.sin_family = AF_INET;
    static int _count{};

    if (bind(sock, (sockaddr *) &address, sizeof address) == -1)exitWithError("Could not bind");
    if (listen(sock, 10) == -1)exitWithError("Could not listen");

    char buf[INET_ADDRSTRLEN];
    memset(buf, 0, INET_ADDRSTRLEN);

    if (inet_ntop(AF_INET, &address.sin_addr, buf, INET_ADDRSTRLEN) == nullptr) {
        exitWithError("Could not log server:host info");
    }

    printf("Listening at %s:%d\n", buf, ntohs(address.sin_port));
    while (true) {
        sockaddr_in addr{};
        len = sizeof addr;
        auto client = accept(sock, (sockaddr *) &addr, &len);
        if (client == -1) {
            exitWithError("Could not accept a client");
        }
        _count++;
        int count = _count;

        memset(buf, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
            exitWithError("Could not log server:host info");
        printf("Accepted client %d from %s:%d\n", count, buf, ntohs(addr.sin_port));

        thread th{[client, count] {
//            auto message = "Hello World!\n";
//            if (send(client, message, strlen(message), 0) == -1)
//                throw FormattedException("Could not say hello");//assuming it sends it all at ounce
            char buf[1000];
            while (true) {
                auto rec = recv(client, buf, sizeof(buf), 0);
                if (rec == -1) {
                    printError("Failed to receive from the client");
                    CLOSE(client);
                    break;
                }
                if (rec == 0) {
                    ::printf("Client %d finished\n", count);
                    CLOSE(client);
                    break;
                } else {
                    ::printf("Read %zd bytes from client %d\n", rec, count);
                    if (string(buf).starts_with("GET / HTTP")) {
                        auto response = "HTTP/1.0 400 Bad Request\n"
                                        "Content-Length: 273\n"
                                        "Date: Sat, 17 Jun 2023 05:32:41 GMT\n"
                                        "\n"
                                        "\n"
                                        "<html><head>\n"
                                        "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
                                        "<title>400 Bad Request</title>\n"
                                        "</head>\n"
                                        "<body text=#000000 bgcolor=#ffffff>\n"
                                        "<h1>Error: Bad Request</h1>\n"
                                        "<h2>Your client has issued a malformed or illegal request.</h2>\n"
                                        "<h2></h2>\n"
                                        "</body></html>";
                        if (send(client, response, strlen(response), 0) == -1)
                            throw FormattedException(
                                    "Could not send http response");//assuming it sends it all at ounce
                        ::printf("Client %d's request has been responded to\n", count);
                        break;
                    }
                }
            }
            CLOSE(client);
        }};
        th.detach();
    }
}

void testSocketReuseAddress() {
    socket_t sock1 = createTcpSocket();
    socket_t sock2 = createTcpSocket();
    socket_t sock3 = createTcpSocket();
    socket_t sock4 = createTcpSocket();
#ifdef  _WIN32
    char optVal = 1;
#else
    int optVal = 1;
#endif
    socklen_t len = sizeof optVal;
    if (setsockopt(sock1, SOL_SOCKET, SO_REUSEADDR, &optVal, len) == -1)
        exitWithError("Could not set reuse address on sock1");
    if (setsockopt(sock2, SOL_SOCKET, SO_REUSEADDR, &optVal, len) == -1)
        exitWithError("Could not set reuse address on sock2");
    if (setsockopt(sock3, SOL_SOCKET, SO_REUSEADDR, &optVal, len) == -1)
        exitWithError("Could not set reuse address on sock3");
    if (setsockopt(sock4, SOL_SOCKET, SO_REUSEADDR, &optVal, len) == -1)
        exitWithError("Could not set reuse address on sock4");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(32212);
    //NO difference between the two? there was difference in linux? can connect without INADDR_ANY
    addr.sin_addr.s_addr = INADDR_ANY;
//    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    if (bind(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not bind sock1");
    if (bind(sock2, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not bind sock2");
    if (bind(sock3, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not bind sock3");//Can not bind after listening starts
//    if (bind(sock4, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
//        exitWithError("Could not bind sock4");

    if (listen(sock2, 20) == -1)exitWithError("Sock2 could not listen");

    addr.sin_port = htons(80);
    inet_pton(AF_INET, "212.53.40.40", &addr.sin_addr.s_addr);
//    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);
    if (connect(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1) {
        exitWithError("Could not connect sock1 to remote server");
    }
    if (send(sock1, "Mr is me", 8, 0) == -1)exitWithError("Could not send on sock1");
    ::printf("Sock1 connected\n");
    CLOSE(sock1);

    sleep(6);

    /*Does not work without this on windows*/
//    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);
    if (connect(sock3, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not connect sock3 to remote server");
    if (send(sock3, "Mr is me", 8, 0) == -1)exitWithError("Could not send on sock3");
    ::printf("Sock3 connected\n");
    CLOSE(sock3);

    sleep(6);

    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(33332);
//    inet_pton(AF_INET, "212.53.40.40", &addr.sin_addr.s_addr);
    if (connect(sock4, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not connect sock4 to the listening socket");

    //Won't work
//    if (connect(sock3, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
//        exitWithError("Could not connect sock3 to the listening socket");

    socket_t accepted{};
    len = sizeof addr;
    if ((accepted = accept(sock2, reinterpret_cast<sockaddr *>(&addr), &len)) == -1)
        exitWithError("Could not accept socket on sock2");
    char addrName[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, addrName, len);//info: not checking for errors
    ::printf("Accepted client from %s:%d\n", addrName, ntohs(addr.sin_port));

    //Won't be able to accept sock3
    if ((accepted = accept(sock2, reinterpret_cast<sockaddr *>(&addr), &len)) == -1)
        exitWithError("Could not accept socket on sock2");
    inet_ntop(AF_INET, &addr.sin_addr, addrName, len);//info: not checking for errors
    ::printf("Accepted client from %s:%d\n", addrName, ntohs(addr.sin_port));
}

void testTCPSocketReconnect() {
#ifdef  _WIN32
    char optVal = 1;
#else
    int optVal = 1;
#endif
    socklen_t len = sizeof optVal;
    socket_t sock1 = createTcpSocket(false);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3334);
    //NO difference between the two
    addr.sin_addr.s_addr = INADDR_ANY;
//    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);


    if (bind(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not bind sock1");

    addr.sin_port = htons(80);
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);

    if (connect(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not connect first time");
    ::printf("Primary session succeeded\n");

    addr.sin_family = AF_UNSPEC;
    if (connect(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not dissolve past connection");
    ::printf("Disconnected successfully\n");
    addr.sin_family = AF_INET;

    if (connect(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr) == -1)
        exitWithError("Could not connect again");
    ::printf("Secondary session succeeded\n");

}

#define cat(a, b) a ## b
#define xCat(a, b) cat(a,b)

#define add(x, y) (x+y)
#define xAdd(x, y) add(x,y)

void testPreProcessor() {
    ::printf("Result of cat %d\n", xCat(cat(1, 2), 3));
    ::printf("Result of add %d\n", add(add(add(1, 1), 1), 1));
}

void testTcpMappedAddress() {
    sockaddr_storage bindAddr{AF_INET};
    reinterpret_cast<sockaddr_in *>(&bindAddr)->sin_port = htons(12121);
    auto address = getTcpMappedAddress(bindAddr);
//    usleep(500000);
//    address = getTcpMappedAddress(bindAddr);

    char addr[100];
    unsigned short port = ntohs((address->ss_family == AF_INET)
                                ? reinterpret_cast<sockaddr_in *>(address.get())->sin_port
                                : reinterpret_cast<sockaddr_in6 *>(address.get())->sin6_port);
    inet_ntop(address->ss_family,
              (address->ss_family == AF_INET) ? (void *) &reinterpret_cast<sockaddr_in *>(address.get())->sin_addr
                                              : (void *) &reinterpret_cast<sockaddr_in6 *>(address.get())->sin6_addr,
              addr, sizeof addr);
    ::printf("Mapped tcp address = %s:%d\n", addr, port);
}

namespace other_blah {
    const char *blah_str2 = "other blah";
    const char *blah_str3 = "other blah";

}
const char *blah_str = "outer";
const char *blah_str2 = "other blah";

namespace blah1 {
    const char *blah_str = "blah1";
    const char *blah_str3 = "other blah";

    namespace blah2 {
        const char *blah_str = "blah2";
        namespace blah3 {//blah3
            const char *blah_str = "blah3";

            void getStr1() {
                blah_str;
            }

            using namespace blah1;

            void getStr2() {
                blah_str;
            }

            using namespace other_blah;

            void getStr3() {
//                blah_str2; //ambiguous, Thus 'using namespace <name>' has equal precedence with the global name space
                blah_str3;
            }
        }
    }
}

void testDynamicAllocation() {
    char c = '1';
    char *cp = &c;
    try {
        delete cp;//Throws an exception that can not be caught
    } catch (exception &alloc) {
        printf("Allocation exception %s\n", alloc.what());
    } catch (...) {
        printf("Allocation exception is not subtype of std::exception\n");
    }
}


void testException() {
    set_terminate([] {
        printf("set_terminate's callback was called\n");
    });
#ifdef _WIN32
    ::signal(SIGTERM, SIG_IGN);
#else
    struct sigaction ig{
            //            [](int){printf("Sig-action's callback was called\n");}
            SIG_IGN
    };
    sigaction(SIGTERM, &ig, nullptr);
#endif


    auto t = thread{[] {
        throw invalid_argument("sdnflksnd");
    }};
    t.detach();
    sleep(5);
    printf("After sleep\n");
}

void testMemoryLeakOnException() {
    int max = 100000;
    for (int ind = 0; ind < max; ind++) {
        try {
            throw FormattedException("Exception message");
        } catch (FormattedException exception) {
            if (ind % (max / 5) == 0) {
                printf("%s\n", exception.what());
                printf("Thrown %d times, %f\n", ind, (float) (ind + 1) / max);
            }
        }
    }
    printf("Finished\n");
    sleep(60);
}

void reuseAddress() {
    sockaddr_in bindAddr{AF_INET, htons(3334)};
    sockaddr_in remoteAddr{AF_INET, htons(80)};
    inet_pton(AF_INET, "212.53.40.40", &remoteAddr.sin_addr);
    while (true) {
        socket_t sock = createTcpSocket(true);
#ifdef LOGGING
        printf("Have generated socket: %d\n", sock);
#endif

        if (bind(sock, reinterpret_cast<const sockaddr *>(&bindAddr), sizeof(sockaddr_in)) == -1) {
            exitWithError("Could not bind socket for curl");
        }
        if (connect(sock, reinterpret_cast<const sockaddr *>(&remoteAddr), sizeof(sockaddr_in)) == -1) {
            exitWithError("Could not connect");
        }
        CLOSE(sock);
        usleep(1000000);
    }

}

void p2pBothTryToConnect(bool random = false) {
    sockaddr_in bindAddress1{AF_INET, htons(random ? 0 : 21324), INADDR_ANY};
    sockaddr_in bindAddress2{AF_INET, htons(random ? 0 : 41371), INADDR_ANY};

    socket_t sock1 = createTcpSocket(true);
    socket_t sock2 = createTcpSocket(true);
    if (bind(sock1, reinterpret_cast<const sockaddr *>(&bindAddress1), sizeof bindAddress1) == -1)
        exitWithError("Could not bind sock1");
    if (bind(sock2, reinterpret_cast<const sockaddr *>(&bindAddress2), sizeof bindAddress2) == -1)
        exitWithError("Could not bind sock2");
    auto mp1 = getTcpMappedAddress(sock1);
    auto mp2 = getTcpMappedAddress(sock2);
    socklen_t len = sizeof bindAddress1;
    if (getsockname(sock1, reinterpret_cast<sockaddr *>(&bindAddress1),
                    &len) == -1)
        throw FormattedException("Could not get bindAddress1");
    len = sizeof bindAddress2;
    if (getsockname(sock2, reinterpret_cast<sockaddr *>(&bindAddress2),
                    &len) == -1)
        throw FormattedException("Could not get bindAddress2");
    CLOSE(sock1);
    CLOSE(sock2);

    sock1 = createTcpSocket(true);
    if (bind(sock1, reinterpret_cast<const sockaddr *>(&bindAddress1), sizeof bindAddress1) == -1)
        exitWithError("Could not bind sock1 at the loop");
    auto t = thread{[&] {
        this_thread::sleep_for(chrono::milliseconds(20000));
        if (connect(sock1, reinterpret_cast<const sockaddr *>(mp2.get()), sizeof(*mp2)) == -1) {
            printError("Sock1 could not connect at attempt "/* + to_string(ind + 1)*/);
        } else printf("Sock1 connected\n");
    }};

    for (int ind = 0; ind < 10; ind++) {
        sock2 = createTcpSocket(true);
        if (bind(sock2, reinterpret_cast<const sockaddr *>(&bindAddress2), sizeof bindAddress2) == -1)
            exitWithError("Could not bind sock2 at the loop");
        if (connect(sock2, reinterpret_cast<const sockaddr *>(mp1.get()), sizeof(*mp1)) == -1) {
            printError("Sock2 could not connect at attempt " + to_string(ind + 1));
        } else printf("Sock2 connected\n");
        CLOSE(sock2);
        usleep(500000);
    }
    t.join();
    CLOSE(sock1);
}

void stunIndicate(socket_t sock) {
    stun::message indicator{stun::message::binding_indication};
    if (send(sock, reinterpret_cast<const char *>(indicator.data()), indicator.size(), 0) == -1)
        throw FormattedException("Could not send stun indication");
}

void testStuntReuseSocket() {
    auto sock = createTcpSocket();
    auto mp1 = getTcpMappedAddress(sock);
//    while(true){
//        stunIndicate(sock);
//        usleep(1);
//    }

    auto mp2 = getTcpMappedAddress(sock, true);
}

void testStunReuseAddress() {
    sockaddr_storage bindAddress{AF_INET};
    reinterpret_cast<sockaddr_in *>(&bindAddress)->sin_port = htons(42120);
    size_t count{};
    while (true) {
//        auto mp = getTcpMappedAddress(bindAddress);

        auto sock = createTcpSocket(true);
        if (bind(sock, reinterpret_cast<const sockaddr *>(&bindAddress), sizeof bindAddress) == -1)
            throw FormattedException("Could not bind");

//        auto mp = getTcpMappedAddress(sock);
        CLOSE(sock);

//        delete mp;
        printf("Resolved public address for the %zu time\n", ++count);
        usleep(5000000);
    }

}

void testClientServerQuery(){

}

int main() {
    initPlatform();
    testTcpMappedAddress();
//    p2pBothTryToConnect(true);
//    startHelloAndListen();
}

#pragma clang diagnostic pop