//
// Created by yoni_ash on 4/15/23.
//
#include <mutex>
#include "packet/TCPPacket.h"

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

struct boolArray {
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
    cout << "With out casting: " << (s << 1) << endl;
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
    bits.a = 2; //under stand from the warning !!!
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
//    inet_pton(AF_INET, "192.168.1.1", &measureConnectTimeAddr.sin_addr.s_addr);
    measureConnectTimeAddr.sin_port = htons(55557);
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

int main() {
    auto timePoint = chrono::steady_clock::now();

    cout << sizeof(timePoint) << endl;
}

#pragma clang diagnostic pop