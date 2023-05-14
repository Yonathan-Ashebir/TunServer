//
// Created by yoni_ash on 4/15/23.
//
#include <mutex>
#include "Include.h"
#include "./packet/TCPPacket.h"
#include "./tunnel/DatagramTunnel.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
using namespace std;

void printSizeC() {
    printf("Char size: %lu\n", sizeof(char));
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

sockaddr_in bindAddress;

void selectTest() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    bindAddress = {AF_INET, 3333};
    bind(listener, (sockaddr *) &bindAddress, sizeof(sockaddr_in));
    listen(listener, 20);


    pthread_t sendThread;


    pthread_create(&sendThread, nullptr, [](void *ptr) -> void * {
        int sender = socket(AF_INET, SOCK_STREAM, 0);
        connect(sender, (sockaddr *) &bindAddress, sizeof(sockaddr_in));
        const char message[] = "How long the country will last is uncertain"
                               "How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain";

        while (true) {
            int sent = send(sender, message, sizeof message, 0);
            if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
//                cout << "Would"
            }
        }
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
        int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (sock < 0)exitWithError("Could not create socket");
        while (true) {
            if (connect(sock, (sockaddr *) &measureConnectTimeAddr, sizeof measureConnectTimeAddr) == -1) {
                printError("Could not connect");
                socklen_t len;
                int err = 0;
                int res = getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);
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
    sockaddr_in addr;
    inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(80);
    addr.sin_family = AF_INET;

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (sock < 0)exitWithError("Could not create socket");
    connect(sock, (sockaddr *) &addr, sizeof addr);
    errno = 0;
    sleep(1);
    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1) {
        exitWithError("Could not connect");
    } else {
        ::printf("Connected Successfully\n");
    }
    char *buf = "GET / HTTP/1.0\r\n\r\n";
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
    int sock1 = socket(AF_INET, SOCK_DGRAM, 0);
    int sock2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock1 < 0 || sock2 < 2)exitWithError("Could not create sockets");

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.4", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3334);
    auto b2 = bind(sock2, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b2 < 0)exitWithError("Could not bind sock2");

    addr.sin_port = htons(3333);
    auto b1 = bind(sock1, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
    if (b1 < 0)exitWithError("Could not bind sock1");

//    int l = listen(sock1, 20);
    thread t{
            [&] {
                ::printf("Waiting at 3333\n");
                char buf[3072];
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
            }
    };


    while (true) {
        char buf[200];
        scanf("%s", buf);
        int s = sendto(sock2, buf, strlen(buf) + 1, 0, (sockaddr *) &addr, sizeof addr);
        if (s < 0)exitWithError("Could not send message");
    }

}

void tunClientTest() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)exitWithError("Could not create sockets");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(41037);

    if (connect(sock, (sockaddr *) &addr, sizeof addr) == -1)exitWithError("Could not connect");
    char msg[] = "HELLO";
    if (send(sock, msg, ::strlen(msg), 0) < strlen(msg))exitWithError("Could not send");
}

[[noreturn]] void udpFlushTest() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)exitWithError("Could not create sockets");

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
    auto sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)exitWithError("Could not create a socket");

    sockaddr_in address{};
    socklen_t len;
    inet_pton(AF_INET, "192.168.1.4", &address.sin_addr);
    address.sin_port = htons(3335);
    address.sin_family = AF_INET;

    if (bind(sock, (sockaddr *) &address, sizeof address) < 0)exitWithError("Could not bind");
    if (listen(sock, 10) < 0)exitWithError("Could not listen");

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
        if (client < 0) {
            exitWithError("Could not accept a client");
        }

        memset(buf, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
            exitWithError("Could not log server:host info");
        printf("Accepted client from %s:%d\n", buf, ntohs(addr.sin_port));

        auto message = "Hello World!\n";

        send(client, message, strlen(message), 0);//assuming it sends it all at ounce
//        sleep(1000);

        close(client);
    }
}

[[noreturn]] void startHelloAndListen() {
    auto sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)exitWithError("Could not create a socket");

    sockaddr_in address{};
    socklen_t len;
    inet_pton(AF_INET, "0.0.0.0", &address.sin_addr);
    address.sin_port = htons(3333);
    address.sin_family = AF_INET;
    static int _count{};

    if (bind(sock, (sockaddr *) &address, sizeof address) < 0)exitWithError("Could not bind");
    if (listen(sock, 10) < 0)exitWithError("Could not listen");

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
        if (client < 0) {
            exitWithError("Could not accept a client");
        }
        _count++;
        int count = _count;

        memset(buf, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
            exitWithError("Could not log server:host info");
        printf("Accepted client %d from %s:%d\n", count, buf, ntohs(addr.sin_port));

        thread th{[client, count] {
            auto message = "Hello World!\n";
            send(client, message, strlen(message), 0);//assuming it sends it all at ounce
            char buf[1000];
            while (true) {
                auto rec = recv(client, buf, sizeof(buf), 0);
                if (rec < 0) {
                    printError("Failed to receive from the client");
                    close(client);
                    break;
                }
                if (rec == 0) {
                    ::printf("Client %d finished\n", count);
                    close(client);
                    break;
                } else {
                    ::printf("Read %zd bytes from client %d\n", rec, count);
                }
            }
            close(client);
        }};
        th.detach();
    }
}


int main() {
    udpFlushTest();
}


#pragma clang diagnostic pop