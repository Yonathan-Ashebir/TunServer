//
// Created by yoni_ash on 5/29/23.
//
#include "ylib-cxx/net/TCPConnector.h"
#include "ConnectionFetcher.h"

using namespace std;

void test1() {
    unsigned short bindPort = 3339;
    sockaddr_storage bindAddr{AF_INET};
    auto *bindAddrIn = reinterpret_cast<sockaddr_in *>(&bindAddr);
    bindAddrIn->sin_port = htons(bindPort);

    TCPConnector<unsigned int> builder{[](TCPSocket &sock, const sockaddr_storage &addr, const unsigned int &info) {
        printf("Socket %d connected\n", sock.getFD());
        sock.close();
    }, [](int errorNum, const sockaddr_storage &addr, const unsigned int &info) {
        printf("Socket with err no %d failed\n", errorNum);
    }};
    builder.setBindPort(bindPort);

    ConnectionFetcher fetcher("the server", "http://yoniash.000webhostapp.com/server.php",
                              [&builder](vector<ConnectRequest> &result) {
                                  ::printf("Fetched %zu connection requests\n", result.size());
                                  for (auto item: result) {
                                      builder.connect(item.addr, item.id);
                                  }
                              });
    fetcher.setOnError([] {
        cout << "Error occurred in fetcher" << endl;
    });
    fetcher.setBindAddress(bindAddr);
    fetcher.start();
    this_thread::sleep_for(chrono::hours(1000));
};

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    test1();
}