//
// Created by yoni_ash on 5/29/23.
//
#include <iostream>
#include "Builder.h"
#include "ConnectionFetcher.h"
#include <curl/curl.h>

using namespace std;

void test1() {
    sockaddr_in bindAddr{AF_INET, htons(3334), INADDR_ANY};

    Builder<unsigned int> builder{[](socket_t sock, sockaddr_in addr, unsigned int info) {
        printf("Socket %d connected\n", sock);
    }, [](socket_t sock, sockaddr_in addr, unsigned int info, int errorNum) {
        printf("Socket %d failed\n", sock);
    }};
    builder.setBindAddress(bindAddr);

    ConnectionFetcher fetcher("the server", [&builder](vector<ResultItem> &result) {
        ::printf("Fetched %zu connection requests\n", result.size());
        for (auto item: result) {
            builder.connect(item.addr, item.id);
        }
    });

    fetcher.setBindAddress(bindAddr);
    fetcher.start("http://yoni-ash.000webhostapp.com/server.php");


};

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    test1();
}