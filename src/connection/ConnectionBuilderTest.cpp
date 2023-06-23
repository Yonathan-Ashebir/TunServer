//
// Created by yoni_ash on 5/29/23.
//
#include "Builder.h"
#include "ConnectionFetcher.h"
#include <curl/curl.h>

using namespace std;

void test1() {
    unsigned short bindPort = 3339;
    sockaddr_storage bindAddr{AF_INET};
    auto *bindAddrIn = reinterpret_cast<sockaddr_in *>(&bindAddr);
    bindAddrIn->sin_port = htons(bindPort);

    Builder<unsigned int> builder{[](TCPSocket &sock, const sockaddr_storage &addr, const unsigned int &info) {
        printf("Socket %d connected\n", sock.getFD());
        sock.close();
    }, [](TCPSocket &sock, const sockaddr_storage &addr, const unsigned int &info, int errorNum) {
        printf("Socket %d failed\n", sock.getFD());
        sock.close();
    }};
    builder.setBindPort(bindPort);

    ConnectionFetcher fetcher("the server", [&builder](vector<ResultItem> &result) {
        ::printf("Fetched %zu connection requests\n", result.size());
        for (auto item: result) {
            builder.connect(item.addr, item.id);
        }
    });

    fetcher.setBindAddress(bindAddr);
    fetcher.start("http://yoniash.000webhostapp.com/server.php");//yoni-ash.000webhostapp.com


};

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    test1();
}