//
// Created by yoni_ash on 6/10/23.
//
#include "../Include.h"
#include "curl/curl.h"
#include <ArduinoJson.h>

#define SERVER_URL "yoniash.000webhostapp.com"
#define  SERVER_RAW_IP "153.92.0.19"
//#define SERVER_URL "localhost:3333"
//#define  SERVER_RAW_IP "0.0.0.0"

struct ResponseData {
    char *buf = new char[len]{};
    size_t len{1000};
    size_t ind{};
};

size_t reader(char *buf, __attribute__((unused)) size_t _, size_t len, ResponseData *responseData) {
    if (responseData->len - responseData->ind < len) {
        responseData->len += len;
        auto newAlloc = ::realloc(responseData->buf, responseData->len);
        if (newAlloc)responseData->buf = static_cast<char *>(newAlloc);
        else
            exitWithError("Could not reallocate response data buffer");
    }
    ::memcpy(responseData->buf, buf, len);
    return len;
}

void test1() {
    curl_global_init(CURL_GLOBAL_ALL);

    DynamicJsonDocument doc{10000};
    char publicIp[INET6_ADDRSTRLEN]{};
    unsigned short publicPort{};
    unsigned int id;

    string serverIp{};
    unsigned short serverPort{};
    unsigned int serverId;
    sockaddr_storage serverAddr{AF_INET};//todo: only  ipv4
    sockaddr_in bindAddress{AF_INET, htons(0)};
//    if (inet_pton(AF_INET, "192.168.1.7", &_selectTestBindAddress.sin_addr) == -1)throw BadException("Could not set bind address");

    TCPSocket stunSocket;
    stunSocket.bind(bindAddress);
    auto publicAddress = getTCPPublicAddress(stunSocket);
    if (inet_ntop(publicAddress->ss_family, (publicAddress->ss_family == AF_INET)
                                            ? (void *) &(reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_addr)
                                            : (void *) &(reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_addr),
                  publicIp, sizeof publicIp) == nullptr) {
        throw BadException("Could not parse the public address");
    }
    publicPort = ntohs((publicAddress->ss_family == AF_INET)
                       ? reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_port
                       : reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_port);
#ifdef LOGGING
    printf("My public address = %s:%d\n", publicIp, publicPort);
#endif
    stunSocket.getBindAddress(bindAddress);
    stunSocket.close();

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        ResponseData data{};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_URL,
                         (string("http://") + SERVER_URL + "/client.php?is_server_query=1").c_str());
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)throw BadException("Could not request for servers");
        deserializeJson(doc, data.buf);
        if (doc.containsKey("servers")) {
            JsonArray arr = doc["servers"];
            if (arr.size() == 0) throw BadException("No servers available");
            JsonObject server = arr[0];
            serverId = server["id"];
            string addrName = server["addr"];
            serverIp = addrName;
            if (inet_pton(AF_INET, serverIp.c_str(), &reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_addr) == -1)
                throw BadException("Bad server ip");
            serverPort = server["port"];
            reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_port = htons(serverPort);

#ifdef LOGGING
            printf("Connected to server with id = %d at %s:%d\n", serverId, serverIp.c_str(), serverPort);
#endif
        } else throw BadException("No 'servers' field provided");

        data.ind = 0;
        auto url = "http://" + string(SERVER_URL) + "/client.php?action=register&addr=" + publicIp +
                   "&port=" + to_string(publicPort) + "&server=" + to_string(serverId);
        curl_easy_setopt(curl, CURLOPT_URL,
                         url.c_str());
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw BadException("Could not register as a client");
        } else {
            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code != 200 && http_code != 404) {
#ifdef LOGGING
                fprintf(stderr, "Request failed: %ld\n",
                        http_code);
#endif

            } else {
                deserializeJson(doc, data.buf);
                if (doc.containsKey("id")) {
                    id = doc["id"];
#ifdef LOGGING
                    printf("Connected to relay server with an id of %d\n", id);
#endif
                }
                auto startTime = chrono::steady_clock::now();
                chrono::milliseconds timeout{30000};
                TCPSocket connector;
                connector.setReuseAddress(true);
                connector.bind(bindAddress);
                while (true) {
                    if (connector.tryConnect(serverAddr) ==
                        -1) {
                        if (chrono::steady_clock::now() - startTime > timeout) {
                            printError("Connection timeout");
                            break;
                        }
                    } else {
                        printf("Connected\n");
                        break;
                    }
                }

            }
        }
    } else exitWithError("curl_easy_init failed");


    curl_global_cleanup();
}

void test2() {
    curl_global_init(CURL_GLOBAL_ALL);

    DynamicJsonDocument doc{10000};
    char publicIp[INET6_ADDRSTRLEN]{};
    unsigned short publicPort{};
    unsigned int id;

    string serverIp{};
    unsigned short serverPort{};
    unsigned int serverId;
    sockaddr_storage serverAddr{AF_INET};//todo: only  ipv4
    sockaddr_in bindAddress{AF_INET, htons(0)};
//    if (inet_pton(AF_INET, "192.168.1.7", &_selectTestBindAddress.sin_addr) == -1)throw BadException("Could not set bind address");

    TCPSocket stunSocket;
    stunSocket.setReuseAddress(true);
    stunSocket.bind(bindAddress);
    auto publicAddress = getTCPPublicAddress(stunSocket);
    if (inet_ntop(publicAddress->ss_family, (publicAddress->ss_family == AF_INET)
                                            ? (void *) &(reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_addr)
                                            : (void *) &(reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_addr),
                  publicIp, sizeof publicIp) == nullptr) {
        throw BadException("Could not parse the public address");
    }
    publicPort = ntohs((publicAddress->ss_family == AF_INET)
                       ? reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_port
                       : reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_port);
#ifdef LOGGING
    printf("My public address = %s:%d\n", publicIp, publicPort);
#endif
    stunSocket.getBindAddress(bindAddress);
//    stunSocket.close();

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        ResponseData data{};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_URL,
                         (string("http://") + SERVER_URL +
                          "/client.php?is_addr_query=1&respondSeparately=1&is_server_query=1&addr=" +
                          publicIp + "&port=" +
                          to_string(publicPort)).c_str());
        thread th{[&] {
            res = curl_easy_perform(curl);
            if (res != CURLE_OK)throw BadException("Could not request for servers");
            printf("Servers http response: %s\n", data.buf);
        }
        };

        sockaddr_storage serverAddr{AF_INET};
        reinterpret_cast<sockaddr_in * >(&serverAddr)->sin_port = htons(33333);
        if (inet_pton(serverAddr.ss_family, SERVER_RAW_IP, &reinterpret_cast<sockaddr_in * >(&serverAddr)->sin_addr) ==
            -1)
            throw BadException("Could not set servers ip");

        /*imp: Do not forget*/
//        auto mp = getTCPPublicAddress(serverAddr);
//        serverAddr = *mp;

        auto startTime = chrono::steady_clock::now();
        chrono::milliseconds timeout{30000};
        TCPSocket reader;
        reader.setReuseAddress(true);
        reader.bind(bindAddress);

        while (true) {
            if (reader.tryConnect(serverAddr) == -1) {
                if (
#ifdef _WIN32
WSAGetLastError() != WSAECONNREFUSED && WSAGetLastError() != WSAETIMEDOUT
#else
errno != ECONNREFUSED && errno != ETIMEDOUT
#endif
                        ) {
                    throw BadException("Could not connect");
                } else if (chrono::steady_clock::now() - startTime > timeout) {
                    printf("Timeout\n");
                    break;
                } else this_thread::sleep_for(chrono::nanoseconds(10000));
            } else {
                printf("Separately Connected\n");
                char buf[10000]{};

//                auto lastReadTime = chrono::steady_clock::now();
//                reader.setBlocking(false);
//                unsigned long amt{};
//                while (true) {
//                    int r = reader.receiveIgnoreWouldBlock(buf, sizeof buf);
//                    if (r > 0) {
//                        amt += r;
//                        lastReadTime = chrono::steady_clock::now();
//                        cout << "Read bytes: " << r << ", total: " << amt << endl;
//
//                    } else if (r == 0) {
//                        cout << "Separate socket closed gracefully after reading bytes: " << amt << endl;
//                        break;
//                    } else {
//                        if (chrono::steady_clock::now() - lastReadTime > chrono::milliseconds(3000)) {
//                            cout << "Separate socket timed out after reading bytes: " << amt << endl;
//                            break;
//                        }
//                    }
//                      this_thread::sleep_for(chrono::nanoseconds(10000));
//                }
//                TCPSocket sock2;
//                sock2.setReuseAddress(true);
//                sock2.bind(bindAddress);
//                sockaddr_in otherStunAddr{AF_INET, htons(3478)};
//                inet_pton(AF_INET, "193.22.17.97", &otherStunAddr.sin_addr);
//                sock2.connect(otherStunAddr);
//                auto r = getTCPPublicAddress(sock2, true);
//                cout << "Public address after: " << getAddressString(*r) << endl;
//                break;

                reader.receive(buf, sizeof buf);
                deserializeJson(doc, buf);
                if (doc.containsKey("servers")) {
                    JsonArray arr = doc["servers"];
                    if (arr.size() == 0) throw BadException("No servers available");
                    JsonObject server = arr[0];
                    serverId = server["id"];
                    string addrName = server["addr"];
                    serverIp = addrName;
                    if (inet_pton(AF_INET, serverIp.c_str(), &reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_addr) ==
                        -1)
                        throw BadException("Bad server ip");
                    serverPort = server["port"];
                    reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_port = htons(serverPort);

#ifdef LOGGING
                    printf("Selected server with id of %d at %s:%d\n", serverId, serverIp.c_str(), serverPort);
#endif
                    TCPSocket connector;
                    while (true)
                        try {
                            connector.connect(addrName.c_str(), serverPort);
                            cout << "Connected successfully" << endl;
                            break;
                        } catch (...) {};
                } else throw BadException("No 'servers' field provided");
                break;
            }
        }
        th.join();

    } else exitWithError("curl_easy_init failed");

    curl_global_cleanup();

}

void test3() {
    curl_global_init(CURL_GLOBAL_ALL);

    DynamicJsonDocument doc{10000};
    char publicIp[INET6_ADDRSTRLEN]{};
    unsigned short publicPort{};
    unsigned int id;

    string serverIp{};
    unsigned short serverPort{};
    unsigned int serverId;
    sockaddr_storage serverAddr{AF_INET};//todo: only  ipv4
    sockaddr_in bindAddress{AF_INET, htons(0)};
//    if (inet_pton(AF_INET, "192.168.1.7", &_selectTestBindAddress.sin_addr) == -1)throw BadException("Could not set bind address");

    TCPSocket stunSocket;
    stunSocket.bind(bindAddress);
    auto publicAddress = getTCPPublicAddress(stunSocket);
    if (inet_ntop(publicAddress->ss_family, (publicAddress->ss_family == AF_INET)
                                            ? (void *) &(reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_addr)
                                            : (void *) &(reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_addr),
                  publicIp, sizeof publicIp) == nullptr) {
        throw BadException("Could not parse the public address");
    }
    publicPort = ntohs((publicAddress->ss_family == AF_INET)
                       ? reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_port
                       : reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_port);
#ifdef LOGGING
    printf("My public address = %s:%d\n", publicIp, publicPort);
#endif
    stunSocket.getBindAddress(bindAddress);
    stunSocket.close();

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        ResponseData data{};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_URL,
                         (string("http://") + SERVER_URL +
                          "/client.php?is_addr_query=1&respondSeparately=1&is_server_query=1&addr=" +
                          publicIp + "&port=" +
                          to_string(publicPort)).c_str());
        thread th{[&] {
            res = curl_easy_perform(curl);
            if (res != CURLE_OK)throw BadException("Could not request for servers");
            printf("Servers http response: %s\n", data.buf);
        }
        };

        sockaddr_storage serverAddr{AF_INET};
        reinterpret_cast<sockaddr_in * >(&serverAddr)->sin_port = htons(33333);
        if (inet_pton(serverAddr.ss_family, SERVER_RAW_IP, &reinterpret_cast<sockaddr_in * >(&serverAddr)->sin_addr) ==
            -1)
            throw BadException("Could not set servers ip");

        /*imp: Do not forget*/
//        auto mp = getTCPPublicAddress(serverAddr);
//        serverAddr = *mp;

        auto startTime = chrono::steady_clock::now();
        chrono::milliseconds timeout{30000};
        TCPSocket reader;
        reader.setReuseAddress(true);
        reader.setBlocking(false);
        reader.bind(bindAddress);

        fd_set writeSet;
        FD_ZERO(&writeSet);
        reader.setIn(writeSet);
        timeval selectTimeout{0, 10000};
        while (true) {
            fd_set copy = writeSet;
            int count = select(reader.getFD() + 1, nullptr, &copy, nullptr, &selectTimeout);
            if (count > 0) {
                int err = reader.getLastError();
                if (err == 0) {
                    printf("Separately Connected\n");
                    char buf[1000]{};
                    reader.setBlocking(true);
                    reader.receive(buf, sizeof buf);
                    deserializeJson(doc, buf);
                    if (doc.containsKey("servers")) {
                        JsonArray arr = doc["servers"];
                        if (arr.size() == 0) throw BadException("No servers available");
                        JsonObject server = arr[0];
                        serverId = server["id"];
                        string addrName = server["addr"];
                        serverIp = addrName;
                        if (inet_pton(AF_INET, serverIp.c_str(),
                                      &reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_addr) ==
                            -1)
                            throw BadException("Bad server ip");
                        serverPort = server["port"];
                        reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_port = htons(serverPort);

#ifdef LOGGING
                        printf("Selected server with id of %d at %s:%d\n", serverId, serverIp.c_str(), serverPort);
#endif
                    } else throw BadException("No 'servers' field provided");
                    break;
                }
                if (
#ifdef _WIN32
err != WSAECONNREFUSED && err != WSAETIMEDOUT
#else
err != ECONNREFUSED && err != ETIMEDOUT
#endif
                        ) {
                    throw BadException("Could not connect");
                } else if (chrono::steady_clock::now() - startTime > timeout) {
                    printf("Timeout\n");
                    break;
                } else {
                    while (true)
                        if (reader.tryConnect(serverAddr) == -1) {
                            if (isConnectionInProgress()) break;
                        }
                    this_thread::sleep_for(chrono::nanoseconds(10000));
                }
            } else if (chrono::steady_clock::now() - startTime > timeout) {
                printf("Timeout\n");
                break;
            } else {
                sockaddr_in anyAddr{AF_UNSPEC};
                while (true) {
                    if (reader.tryConnect(serverAddr) == -1) {
                        if (isConnectionInProgress()) break;
                    }
                }
                this_thread::sleep_for(chrono::nanoseconds(1000));
            }
        }
        th.join();

    } else exitWithError("curl_easy_init failed");

    stunSocket.send("blah", 4);
    curl_global_cleanup();
}

int main() {
    initPlatform();
    test2();

}