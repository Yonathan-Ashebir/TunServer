//
// Created by yoni_ash on 4/15/23.
//
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "Include.h"
#include "./packet/TCPPacket.h"
#include "./tunnel/DatagramTunnel.h"
#include "Test.h"
#include "./sessions2/TCPSession.h"

using namespace std;

#define ipAddr "127.0.0.1"

void printSizeC() {
    printf("Char capacity: %llu\n", sizeof(char));
}


void sizeOfArrayAndElement() {
    char arr[30];
    typeof(arr + 1) el;
    printf("Arr capacity: %lu and its element: %lu\n", sizeof(arr), sizeof(el));
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
    TCPSocket listener;
    listener.setReuseAddress(true);
    _selectTestBindAddress = {AF_INET, 33333};
    listener.bind(_selectTestBindAddress);
    listener.listen();

    pthread_t sendThread;
    pthread_create(&sendThread, nullptr, [](void *ptr) -> void * {
        TCPSocket sender;
        inet_pton(AF_INET, "127.0.0.1", &_selectTestBindAddress.sin_addr);
        sender.connect(_selectTestBindAddress);
        const char message[] = "How long the country will last is uncertain"
                               "How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain""How long the country will last is uncertain";

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
        while (true) {
            sender.send(message, sizeof message);
            if (isWouldBlock()) {
                cout << "Would block" << endl;
            } else if (errno != 0) {
                break;
            }
        }
#pragma clang diagnostic pop
    }, nullptr);

    auto sock = listener.accept(_selectTestBindAddress);
    char buf[1000];
    while (true) {
        int r = sock.receive(buf, sizeof buf);
        cout << "Read bytes: " << r << endl;
        this_thread::sleep_for(chrono::milliseconds (1000));
    }
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

void measureCPUTime(function<void()> call) {
    ::clock_t start, end;
    start = ::clock();
    call();
    end = ::clock();
    long diff = end - start;
    printf("Spent %ld clocks or %ld millis\n", diff, diff / CLOCKS_PER_SEC * 1000);
}

void measureElapsedTime(function<void()> call) {
    auto start_time = std::chrono::high_resolution_clock::now();
    call();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    printf("Spent %ld millis\n", time / std::chrono::milliseconds(1));
}


void measureConnectTime() {
    sockaddr_storage measureConnectTimeAddr{};
    auto call = [&] {
        auto result = resolveHost("www.google.com", "80");
        measureConnectTimeAddr = *(sockaddr_storage *) result->ai_addr;
        cout << "Public ip: " << getAddressString(measureConnectTimeAddr) << endl;
    };
    measureElapsedTime(call);

    char c;
    cin >> c;
    measureElapsedTime([&] {
        TCPSocket sock;
        while (true) {
            if (sock.tryConnect(measureConnectTimeAddr) == -1) {
                printError("Could not connect");
                printError("Socket option error", sock.getLastError());
            } else {
                ::printf("Connected Successfully\n");
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

    TCPSocket sock;
    sock.connect(addr);
    ::printf("Connected Successfully\n");

    const char *buf = "GET / HTTP/1.0\r\n\r\n";
    size_t total = sock.send(buf, 0);
    printError("After send");
    cout << "Total: " << total << endl;

    total = sock.send(buf, strlen(buf), 0);
    printError("After send");
    ::printf("Total: %zu\n", total);
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
    UDPSocket sock1;
    UDPSocket sock2;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.4", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3334);
    sock2.bind(addr);

    addr.sin_port = htons(3333);
    sock1.bind(addr);

//    int l = listen(sock1, 20);
    thread t{
            [&] {
                ::printf("Waiting at 3333\n");
                char buf[3072];
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
                while (true) {
                    sockaddr_in from{};
                    auto r = sock1.receiveFrom(buf, sizeof buf, from);
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &from.sin_addr.s_addr, ip, sizeof ip);
                    printf("Received %zd bytes from %s:%d\n", r, ip, ntohs(from.sin_port));
                }
#pragma clang diagnostic pop
            }
    };


    while (true) {
        char buf[200];
        scanf("%s", buf);
        errno = 0;
        size_t s = sock2.sendTo(buf, strlen(buf) + 1, addr);
        if (errno != 0)exitWithError("Could not send the message");
    }

}

void tunClientTest() {
    UDPSocket sock;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(3333);

    sock.connect(addr);
    char msg[] = "HELLO";
    if (sock.send(msg, (int) ::strlen(msg)) < strlen(msg))throw length_error("Could not send it all");
}


[[noreturn]] void udpFlushTest() {
    UDPSocket sock;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(80);

    sock.connect(addr);
    char msg[5000]{'H'};
    auto msgStr = string(5000, 'H');

    TCPPacket packet(3072);
    DatagramTunnel tunnel(sock);
    while (true) {
#define msg_udp_flush msgStr.c_str()
        if (sock.send(msg_udp_flush, ::strlen(msg_udp_flush)) < strlen(msg_udp_flush))
            throw length_error("Could not send it all");

//        packet.makeNormal(100, 100);
//        packet.setWindowSize(65535);
//        packet.clearData();
//        packet.setPayload(reinterpret_cast<unsigned char *>(msg), 516);
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
    TCPSocket sock;
    sockaddr_in address{};
    socklen_t len;
    inet_pton(AF_INET, "192.168.1.4", &address.sin_addr);
    address.sin_port = htons(3335);
    address.sin_family = AF_INET;

    sock.bind(address);
    sock.listen();

    char buf[INET_ADDRSTRLEN];
    memset(buf, 0, INET_ADDRSTRLEN);

    if (inet_ntop(AF_INET, &address.sin_addr, buf, INET_ADDRSTRLEN) == NULL) {
        exitWithError("Could not log server:host info");
    }

    printf("Listening at %s:%d\n", buf, ntohs(address.sin_port));
    while (true) {
        sockaddr_in addr{};
        auto client = sock.accept(addr);
        memset(buf, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
            exitWithError("Could not log server:host info");
        printf("Accepted client from %s:%d\n", buf, ntohs(addr.sin_port));
        auto message = "Hello World!\n";
        client.send(message, strlen(message));//assuming it sends it all at ounce
//       this_thread::sleep_for(chrono::seconds(1000));
    }
}

[[noreturn]] void startHelloAndListen() {
    TCPSocket sock;

    sockaddr_in address{};
    socklen_t len;
    inet_pton(AF_INET, "0.0.0.0", &address.sin_addr);
    address.sin_port = htons(3333);
    address.sin_family = AF_INET;
    static int _count{};

    sock.bind(address);
    sock.listen();

    char buf[INET_ADDRSTRLEN];
    memset(buf, 0, INET_ADDRSTRLEN);

    if (inet_ntop(AF_INET, &address.sin_addr, buf, INET_ADDRSTRLEN) == nullptr) {
        exitWithError("Could not log server:host info");
    }

    printf("Listening at %s:%d\n", buf, ntohs(address.sin_port));
    while (true) {
        sockaddr_in addr{};
        TCPSocket client = sock.accept(addr);
        _count++;
        int count = _count;

        memset(buf, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN) == NULL)
            exitWithError("Could not log server:host info");
        printf("Accepted client %d from %s:%d\n", count, buf, ntohs(addr.sin_port));

        thread th{[&, client] {
            TCPSocket cl = client;
//            auto message = "Hello World!\n";
//            if (send(client, message, strlen(message), 0) == -1)
//                throw BadException("Could not say hello");//assuming it sends it all at ounce
            char buf[1000];
            while (true) {
                auto rec = cl.tryReceive(buf, sizeof(buf), 0);
                if (rec == -1) {
                    printError("Failed to receive from the client");
                    break;
                }
                if (rec == 0) {
                    ::printf("Client %d finished\n", count);
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
                        cl.send(response, strlen(response));
                        ::printf("Client %d's request has been responded to\n", count);
                        break;
                    }
                }
            }
        }};
        th.detach();
    }
}

void testSocketReuseAddress() {
    TCPSocket sock1;
    TCPSocket sock2;
    TCPSocket sock3;
    TCPSocket sock4;


    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(37932);
    //NO difference between the two? there was difference in linux? can connect without INADDR_ANY
    addr.sin_addr.s_addr = INADDR_ANY;
//    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    sock1.bind(addr);
    sock2.bind(addr);
    sock3.bind(addr);
//    sock4.bind(addr);

    sock2.listen();

    addr.sin_port = htons(80);
    inet_pton(AF_INET, "212.53.40.40", &addr.sin_addr.s_addr);
//    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);
    sock1.connect(addr);

    unsigned char tx[12];
    stun::message message{STUN_BINDING_REQUEST, tx};
    sock1.send(message.data(), message.size());
    ::printf("Sock1 connected\n");
    sock1.close();

   this_thread::sleep_for(chrono::seconds(1));

    /*Does not work without this on windows*/
//    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);
    sock3.connect(addr);
    sock3.send("Mr is me", 8);
    ::printf("Sock3 connected\n");
    sock3.close();

//   this_thread::sleep_for(chrono::seconds(6));

    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(33332);
//    inet_pton(AF_INET, "212.53.40.40", &addr.sin_addr.s_addr);
    sock4.connect(addr);

    //Won't work
    sock3.connect(addr);

    TCPSocket accepted = sock2.accept(addr);
    char addrName[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, addrName, sizeof addrName);//info: not checking for errors
    ::printf("Accepted client from %s:%d\n", addrName, ntohs(addr.sin_port));
}

void testTCPSocketReconnect() {
    TCPSocket sock1;

    sockaddr_in addr{AF_INET, htons(3334), INADDR_ANY};
//    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    sock1.bind(addr);

    addr.sin_port = htons(80);
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);

    sock1.connect(addr);
    ::printf("Primary session succeeded\n");

    addr.sin_family = AF_UNSPEC;
    sock1.connect(addr);
    ::printf("Disconnected successfully\n");
    addr.sin_family = AF_INET;

    sock1.connect(addr);;
    ::printf("Secondary session succeeded\n");
}

void testTCPSocketRetryConnect() {
    TCPSocket sock1;
    sock1.setReuseAddress(true);

    sockaddr_in addr{AF_INET, htons(3337), INADDR_ANY};
//    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    sock1.bind(addr);

    addr.sin_port = htons(800);
    inet_pton(AF_INET, "212.53.40.40", &addr.sin_addr.s_addr);
    auto startTime = chrono::steady_clock::now();
    chrono::milliseconds timeout{30000};

    while (true) {
        if (sock1.tryConnect(addr) == -1) {
#ifdef _WIN32
            auto err = WSAGetLastError();
            if (err == WSAECONNREFUSED || err == WSAETIMEDOUT)
#else
                if (errno == ECONNREFUSED || errno == ETIMEDOUT)
#endif
            {
                if (chrono::steady_clock::now() - startTime > timeout) {
                    printf("Timed out");
                    break;
                }
            } else throw SocketError("Could not connect");
        } else {
            printf("Connected successfully");
            break;
        }
    }
}

#define cat(a, b) a ## b
#define xCat(a, b) cat(a,b)

#define add(x, y) (x+y)
#define xAdd(x, y) add(x,y)

void testPreProcessor() {
    ::printf("Result of cat %d\n", xCat(cat(1, 2), 3));
    ::printf("Result of add %d\n", add(add(add(1, 1), 1), 1));
}

void testTCPMappedAddress() {
    sockaddr_storage bindAddr{AF_INET};
    reinterpret_cast<sockaddr_in *>(&bindAddr)->sin_port = htons(11421);
    auto address = getTCPPublicAddress(bindAddr);
//      this_thread::sleep_for(chrono::nanoseconds(500000));
//    address = getTCPPublicAddress(bindAddr);

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

void testUDPMappedAddress() {
    sockaddr_storage bindAddr{AF_INET};
    reinterpret_cast<sockaddr_in *>(&bindAddr)->sin_port = htons(12421);
    auto address = getUDPPublicAddress(bindAddr);
//      this_thread::sleep_for(chrono::nanoseconds(500000));
//    address = getTCPPublicAddress(bindAddr);

    char addr[100];
    unsigned short port = ntohs((address->ss_family == AF_INET)
                                ? reinterpret_cast<sockaddr_in *>(address.get())->sin_port
                                : reinterpret_cast<sockaddr_in6 *>(address.get())->sin6_port);
    inet_ntop(address->ss_family,
              (address->ss_family == AF_INET) ? (void *) &reinterpret_cast<sockaddr_in *>(address.get())->sin_addr
                                              : (void *) &reinterpret_cast<sockaddr_in6 *>(address.get())->sin6_addr,
              addr, sizeof addr);
    ::printf("Mapped udp address = %s:%d\n", addr, port);
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
   this_thread::sleep_for(chrono::seconds(5));
    printf("After sleep\n");
}

void testMemoryLeakOnException() {
    int max = 100000;
    for (int ind = 0; ind < max; ind++) {
        try {
            throw BadException("Exception message");
        } catch (BadException e) {
            if (ind % (max / 5) == 0) {
                printf("%s\n", e.what());
                printf("Thrown %d times, %f\n", ind, (float) (ind + 1) / max);
            }
        }
    }
    printf("Finished\n");
   this_thread::sleep_for(chrono::seconds(60));
}

void reuseAddress() {
    sockaddr_in bindAddr{AF_INET, htons(3334)};
    sockaddr_in remoteAddr{AF_INET, htons(8)};
    inet_pton(AF_INET, "212.53.40.40", &remoteAddr.sin_addr);
    while (true) {
        TCPSocket sock;
#ifdef LOGGING
        printf("Have generated socket: %d\n", sock.getFD());
#endif

        sock.bind(bindAddr);
        sock.connect(remoteAddr);
        sock.close();
          this_thread::sleep_for(chrono::nanoseconds(1000000));
    }

}

void p2pBothTryToConnect(bool random = false) {
    sockaddr_in bindAddress1{AF_INET, htons(random ? 0 : 21324), INADDR_ANY};
    sockaddr_in bindAddress2{AF_INET, htons(random ? 0 : 41371), INADDR_ANY};

    TCPSocket sock1;
    TCPSocket sock2;
    sock1.setReuseAddress(true);
    sock2.setReuseAddress(true);
    sock1.bind(bindAddress1);
    sock2.bind(bindAddress2);

    auto mp1 = getTCPPublicAddress(sock1);
    auto mp2 = getTCPPublicAddress(sock2);
    socklen_t len = sizeof bindAddress1;
    sock1.getBindAddress(bindAddress1);
    sock2.getBindAddress(bindAddress2);
    sock1.close();
    sock2.close();

    sock1.regenerate();
    sock1.setReuseAddress(true);
    sock1.bind(bindAddress1);
    auto t = thread{[&] {
//        this_thread::sleep_for(chrono::milliseconds(20000));
        if (sock1.tryConnect(*mp2) == -1) {
            printError("Sock1 could not connect at attempt "/* + to_string(ind + 1)*/);
        } else printf("Sock1 connected\n");
    }};

    sock2.regenerate();
    sock2.setReuseAddress(true);
    sock2.bind(bindAddress2);

    for (int ind = 0; ind < 10; ind++) {
        if (sock2.tryConnect(*mp1) == -1) {
            printError("Sock2 could not connect at attempt " + to_string(ind + 1));
        } else {
            printf("Sock2 connected\n");
            break;
        }
          this_thread::sleep_for(chrono::nanoseconds(500000));
    }
    t.join();
}

void stunIndicate(socket_t sock) {
    stun::message indicator{stun::message::binding_indication};
    if (send(sock, reinterpret_cast<const char *>(indicator.data()), indicator.size(), 0) == -1)
        throw BadException("Could not send stun indication");
}

void testStunReuseSocket() {
    TCPSocket sock;
    auto mp1 = getTCPPublicAddress(sock);
//    while(true){
//        stunIndicate(sock);
//          this_thread::sleep_for(chrono::nanoseconds(1));
//    }

    auto mp2 = getTCPPublicAddress(sock, true);
}

void testStunReuseAddress() {
    sockaddr_storage bindAddress{AF_INET};
    reinterpret_cast<sockaddr_in *>(&bindAddress)->sin_port = htons(32310);
    linger ln{1, 0};
    sockaddr_in otherStunAddr{AF_INET, htons(3478)};
    inet_pton(AF_INET, "193.22.17.97", &otherStunAddr.sin_addr);
    for (int count = 0; count < 20; count++) {
        unique_ptr<sockaddr_storage> mp;
        if (count % 2 == 1) {
            mp = getTCPPublicAddress(bindAddress);
        } else {
            TCPSocket sock;
            sock.setLinger(ln);
            sock.bind(bindAddress);
            sock.connect(otherStunAddr);
            throw BadException("Could not connect to the other stun server");
            mp = getTCPPublicAddress(sock, true);
        }

        printf("Resolved public address of %s, trier %d\n", getAddressString(*mp).c_str(), count + 1);
          this_thread::sleep_for(chrono::nanoseconds(2000000));
    }

}

void testClientServerQuery() {

}

class A {
public:
    int i{};
};

void testConst() {
    const A a;
//    a.i = 4; //Not allowed
    A b;
//    a = b; // Also not, if opeerator not overrieden
    b = b;
    A &&c = move(b);
    c.i = 5;

    char *const string1 = "hello";
    string1[0] = 'b';
//    string1 = "Bye"; //Not allowed

    const char *string2 = "hello";
//    string2[0] = 'b'; //Not alloed
    string2 = "lo";
}

void testTypeId() {
    char c;
    auto t = chrono::steady_clock::now();
    cout << typeid(t).name() << endl;
}

void testStringOverload(string str) {//
    printf("The one with string is called\n");
}

void testStringOverload(char *str) {//
    printf("The one with char * is called\n");
}

void testStringOverload(const char *str) {//
    printf("The one with const char * is called\n");
}

void testStringLiteralEquality() {
    char *literal1 = "hello";
    char arr[] = "hello"; //can not use literal1 to init arr
    const char *literal2 = "hello";
    printf("Arr == literal: %d and literal == literal: %d\n", arr == literal1, literal1 == literal2);
}

class Parent {
    int a;
public:
    Parent() {};

    virtual ~Parent() {
        printf("Parent's descructor was called\n");
    }

    Parent(Parent &p) {
        cout << "Parent copy constructor called" << endl;

    }

    Parent(Parent &&p) {
        cout << "Parent move constructor called" << endl;
    }


    virtual Parent &operator=(Parent &) {
        cout << "Parent copy called" << endl;
    }

    virtual Parent &operator=(Parent &&) {
        cout << "Parent move called" << endl;
    }

    virtual void method() {
        cout << "Parent's method called" << endl;
    }
};

class Child : public Parent {
    int b;
public:
    explicit Child() : Parent() {};

    ~Child() override {
        printf("Child's descructor was called\n");
    }

    virtual Child &operator=(Child &) {
        cout << "Child copy called" << endl;
    }

    virtual Child &operator=(Child &&) {
        cout << "Child move called" << endl;
    }

    void method() {
        cout << "Child's method called" << endl;
    }
};

class GrandChild : public Child {
    ~GrandChild() {
        printf("Grand child's destructor was called\n");
    }

    void method() {
        cout << "Grand child's method called" << endl;
    }
};

void testInheritanceOfCopyMoveOperators() {
    Child c1;
    auto method = [](Child c) {};
//    method(c1);
//    Child c2 = c1;
//    method(move(c1));
//    Child c2 = move(c1); //Won't work with out the constructors: constructors vs operators

    Child c3;
    c3 = c1; //Won't work with out the operator in child: operators inheritance, child's

    Parent &p1 = c1;
    Parent p3;
    p3 = p1;//call parent's
    p3 = c1;//call parent's
    p1 = c1;//call parent's
    c3 = move(c1);//child's
    //Won't work with out the operator in child
}

void testDestructor() {
//    Child ch; //calls all descturctors, no virtual needed

    Parent *p = new GrandChild; //calls all, virtual needed
    p->method(); // calls the grand chlild's method, no need to explicitly mark child's method viertual
    delete p;
}

void testFakeReturn() {
    auto noReturn = [] -> int {};
    int count = 1;
    for (int ind = 0; ind < 100; ind++) {
        if (noReturn() != 0) {
            printf("Non zero return: %d count: %d\n", noReturn(), count++);
        } else
//            printf("Zero returned\n");
              this_thread::sleep_for(chrono::nanoseconds(10000));
    }
}

struct VariableBufferStruct {
    static int size;
//    char buffer[capacity];//Won't work unless (contexpr / const) static
};

void emptyArrayPointerTest() {
    char *buf = new char[0];
    char *buf2 = new char[0];
    printf("Ptr1 0x%lX\n", buf);
    printf("Ptr2 0x%lX\n", buf2);
    delete buf;
    delete (buf2);
    cout << "After" << endl;
}

void artimetryOnPointerTest() {
    char *n = nullptr;
    char *a = (char *) 1;
    char *b = (char *) 2;
    int *i = (int *) 2;
    int *i2 = (int *) 3;
    int *i3 = (int *) 7;
    void *v1 = i2;
    void *v2 = i3;
    printf("Ptr 0x%llX\n", a - n);
    printf("Ptr 0x%llX\n", b - a);
//    printf("Ptr 0x%lX\n", i - n ); //Not valid
    printf("Ptr 0x%llX\n", i2 - i); //Prints 0
    printf("Ptr 0x%llX\n", i3 - i2); //Prints 1
    printf("Ptr 0x%llX\n", i3 - i); //Prints 1
    printf("Ptr 0x%llX\n", i3 - i); //Prints 1
//    printf("Ptr 0x%llX\n", i3 + i); //Not valid
//    printf("Ptr 0x%llX\n", v2 - v1); //Not valid


}

void testPrintAddress() {
    printf("Ptr1 0x%lX\n", printf);
    printf("Ptr1 0x%lX\n", &printf);
//   this_thread::sleep_for(chrono::seconds(100));
}

struct CompressedTCPH {
    unsigned short sourcePort{};
    unsigned short destinationPort{};
    unsigned int sequenceNumber{};
    unsigned int acknowledgementNumber{};
    char cwr: 1{};
    char ece: 1{};
    char urg: 1{};
    char ack: 1{};
    char psh: 1{};
    char rst: 1{};
    char syn: 1{};
    char fin: 1{};
    unsigned short window{};
};

/*Bit order consideration of wire shark, clion and the system are the same, but according to tcp documentation and the fact that fin is set at the last bit in wireshark similarily, thus structs in my linux system are reverse ordered, and the system's byte order is similar to the networks*/
void testBitFields() {
    int a = 1;
    CompressedTCPH header{1, 2, 3, 4, 1, 1, 1, 1, 0, 0, 0, 0, 5};
    tcphdr header2{};
    header2.fin = 1;
    printf("Size of header struct: %d\n", sizeof header);
    TCPSocket sock;
    sock.connect("1.1.1.1", 80);
    sock.sendObject(header2);
}

extern void testInlineVirtual2();

void testInlineVirtual() {
    InlineVirtualChild ch;
    InlineVirtualParent *chPtr = &ch;
    ch.fun1();
}

void InlineVirtualParent::fun1() { cout << "Parent's called" << endl; }

void InlineVirtualChild::fun1() { cout << "Child's called" << endl; }

void testReadBuffer() {
    char str[80], ch;

    // scan input from user -
    // GeeksforGeeks for example
    scanf("%s", str);

    scanf("%s", str);
    cout << str << endl;
    // flushes the standard input
    // (clears the input buffer)
    char c;
    while ((c = getchar()) != '\n') { printf("Char code in loop %d\n", c); };

    // scan character from user -
    // 'a' for example
    ch = getchar();

    // Printing character array,
    // prints “GeeksforGeeks”)
    printf("%s\n", str);

    // Printing character a: It
    // will print 'a' this time
    printf("%c", ch);
}

template<class T>
void testTypeComparitionHelper() {
    printf("Result: %d\n", typeid(T) == typeid(char));
}

void testTypeComparition() {
    testTypeComparitionHelper<char>();
    typedef char blah;
    testTypeComparitionHelper<blah>();
    struct blah2 {
        char a1: 1;
    };
    testTypeComparitionHelper<blah2>();
}

void testMutex() {
    mutex mtx;
//    mtx.lock();
    unique_lock<mutex> lock(mtx); //blocks
    unique_lock<mutex> lock2(mtx); //blocks
//    mtx.lock(); //blocks
    printf("Test passed\n");
}

void testAsync() {
    auto f = async([] {
       this_thread::sleep_for(chrono::seconds(1));
        cout << "After 1 sec" << endl;
       this_thread::sleep_for(chrono::seconds(1));
        cout << "After 2 sec" << endl;
    });
}

void testThreadHandleDestruct() {
    thread t{[] {
       this_thread::sleep_for(chrono::seconds(1));
        cout << "After 1 sec" << endl;
       this_thread::sleep_for(chrono::seconds(1));
        cout << "After 2 sec" << endl;
    }};
    t.detach();
}

struct ConstructorOnDeclarationStruct {
    TCPSocket sock;
    int a;

    ConstructorOnDeclarationStruct() {};
};


void testConstructionOnDeclaration() {
    ConstructorOnDeclarationStruct data;
}

void testOverFlowArtimetry() {
    int a = 2 * numeric_limits<int>::max();
    int b = -1;
    cout << "Result of b - a: " << (b - a) << endl;

    unsigned int c{1}, d{2};
    auto res = c - d; //ha! no always false warrning even with a variable with well defined type
    cout << "Result of (c - d < 0): " << (res < 0) << endl;
}

int main() {
    initPlatform();

    int arr[5] = {4};
    testOverFlowArtimetry();
}

#pragma clang diagnostic pop
