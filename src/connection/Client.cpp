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
//    if (inet_pton(AF_INET, "192.168.1.7", &_selectTestBindAddress.sin_addr) == -1)throw FormattedException("Could not set bind address");

    socket_t stunSocket = createTcpSocket(true);
    if (bind(stunSocket, reinterpret_cast<const sockaddr *>(&bindAddress), sizeof bindAddress) == -1)
        throw FormattedException("Could not bind stun socket");
    auto publicAddress = getTcpMappedAddress(stunSocket);
    if (inet_ntop(publicAddress->ss_family, (publicAddress->ss_family == AF_INET)
                                            ? (void *) &(reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_addr)
                                            : (void *) &(reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_addr),
                  publicIp, sizeof publicIp) == nullptr) {
        throw FormattedException("Could not parse the public address");
    }
    publicPort = ntohs((publicAddress->ss_family == AF_INET)
                       ? reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_port
                       : reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_port);
#ifdef LOGGING
    printf("My public address = %s:%d\n", publicIp, publicPort);
#endif
    socklen_t len = sizeof bindAddress;
    if (getsockname(stunSocket, reinterpret_cast<sockaddr *>(&bindAddress),
                    &len) == -1)
        throw FormattedException("Could not get _selectTestBindAddress");
    CLOSE(stunSocket);
    CURL *curl;
    CURLcode res;

    /* In windows, this will init the winsock stuff */

    curl = curl_easy_init();
    if (curl) {
        ResponseData data{};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_URL,
                         (string("http://") + SERVER_URL + "/client.php?is_server_query=1").c_str());
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)throw FormattedException("Could not request for servers");
        deserializeJson(doc, data.buf);
        if (doc.containsKey("servers")) {
            JsonArray arr = doc["servers"];
            if (arr.size() == 0) throw FormattedException("No servers available");
            JsonObject server = arr[0];
            serverId = server["id"];
            string addrName = server["addr"];
            serverIp = addrName;
            if (inet_pton(AF_INET, serverIp.c_str(), &reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_addr) == -1)
                throw FormattedException("Bad server ip");
            serverPort = server["port"];
            reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_port = htons(serverPort);

#ifdef LOGGING
            printf("Connected to server with id = %d at %s:%d\n", serverId, serverIp.c_str(), serverPort);
#endif
        } else throw FormattedException("No 'servers' field provided");

        data.ind = 0;
        auto url = "http://" + string(SERVER_URL) + "/client.php?action=register&addr=" + publicIp +
                   "&port=" + to_string(publicPort) + "&server=" + to_string(serverId);
        curl_easy_setopt(curl, CURLOPT_URL,
                         url.c_str());
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw FormattedException("Could not register as a client");
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
                while (true) {
                    socket_t connector = createTcpSocket(true);
                    if (bind(connector, reinterpret_cast<const sockaddr *>(&bindAddress), sizeof bindAddress) == -1)
                        throw FormattedException("Could not bind the connector");
                    if (connect(connector, reinterpret_cast<const sockaddr *>(&serverAddr), sizeof serverAddr) ==
                        -1) {
                        if (chrono::steady_clock::now() - startTime > timeout) {
                            printError("Connection timeout");
                            CLOSE(connector);
                            break;
                        }
                    } else {
                        printf("Connected\n");
                        CLOSE(connector);
                        break;
                    }
                    CLOSE(connector);
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
//    if (inet_pton(AF_INET, "192.168.1.7", &_selectTestBindAddress.sin_addr) == -1)throw FormattedException("Could not set bind address");

    socket_t stunSocket = createTcpSocket(true);
    if (bind(stunSocket, reinterpret_cast<const sockaddr *>(&bindAddress), sizeof bindAddress) == -1)
        throw FormattedException("Could not bind stun socket");
    auto publicAddress = getTcpMappedAddress(stunSocket);
    if (inet_ntop(publicAddress->ss_family, (publicAddress->ss_family == AF_INET)
                                            ? (void *) &(reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_addr)
                                            : (void *) &(reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_addr),
                  publicIp, sizeof publicIp) == nullptr) {
        throw FormattedException("Could not parse the public address");
    }
    publicPort = ntohs((publicAddress->ss_family == AF_INET)
                       ? reinterpret_cast<sockaddr_in *>(publicAddress.get())->sin_port
                       : reinterpret_cast<sockaddr_in6 *>(publicAddress.get())->sin6_port);
#ifdef LOGGING
    printf("My public address = %s:%d\n", publicIp, publicPort);
#endif
    socklen_t len = sizeof bindAddress;
    if (getsockname(stunSocket, reinterpret_cast<sockaddr *>(&bindAddress),
                    &len) == -1)
        throw FormattedException("Could not get _selectTestBindAddress");
    CLOSE(stunSocket);
    CURL *curl;
    CURLcode res;

    /* In windows, this will init the winsock stuff */

    curl = curl_easy_init();
    if (curl) {
        ResponseData data{};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_URL,
                         (string("http://") + SERVER_URL + "/client.php?respondSeparately=1&is_server_query=1&addr=" + publicIp + "&port=" +
                          to_string(publicPort)).c_str());
        thread th{[&] {
            res = curl_easy_perform(curl);
            if (res != CURLE_OK)throw FormattedException("Could not request for servers");
            printf("Servers http response: %s\n", data.buf);
        }
        };

        sockaddr_storage serverAddr{AF_INET};
        reinterpret_cast<sockaddr_in* >(&serverAddr)->sin_port = htons(33333);
        if (inet_pton(serverAddr.ss_family, SERVER_RAW_IP, &reinterpret_cast<sockaddr_in* >(&serverAddr)->sin_addr) == -1)
            throw FormattedException("Could not set servers ip");

        /*imp: Do not forget*/
//        auto mp = getTcpMappedAddress(serverAddr);
//        serverAddr = *mp;

        auto startTime = chrono::steady_clock::now();
        chrono::milliseconds timeout{30000};
        while (true) {
            socket_t reader = createTcpSocket(true);
            if (bind(reader, reinterpret_cast<const sockaddr *>(&bindAddress), sizeof bindAddress) == -1)
                throw FormattedException("Could not bind reader");
            if (connect(reader, reinterpret_cast<const sockaddr *>(&serverAddr), sizeof serverAddr) == -1) {
                if (
#ifdef _WIN32
WSAGetLastError() != WSAECONNREFUSED
#else
errno != ECONNREFUSED
#endif
                        ) {
                    throw FormattedException("Could not connect");
                } else if (chrono::steady_clock::now() - startTime > timeout) {
                    printf("Timeout\n");
                    break;
                }
            } else {
                printf("Connected\n");
                char buf[1000]{};
                if (read(reader, buf, sizeof buf) == -1)throw FormattedException("Could not read");
                deserializeJson(doc, buf);
                if (doc.containsKey("servers")) {
                    JsonArray arr = doc["servers"];
                    if (arr.size() == 0) throw FormattedException("No servers available");
                    JsonObject server = arr[0];
                    serverId = server["id"];
                    string addrName = server["addr"];
                    serverIp = addrName;
                    if (inet_pton(AF_INET, serverIp.c_str(), &reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_addr) ==
                        -1)
                        throw FormattedException("Bad server ip");
                    serverPort = server["port"];
                    reinterpret_cast<sockaddr_in *>(&serverAddr)->sin_port = htons(serverPort);

#ifdef LOGGING
                    printf("Connected to server with id = %d at %s:%d\n", serverId, serverIp.c_str(), serverPort);
#endif
                } else throw FormattedException("No 'servers' field provided");
                break;
            }
            CLOSE(reader);
        }
        th.join();

    } else exitWithError("curl_easy_init failed");


    curl_global_cleanup();
}

int main() {
    initPlatform();
    test2();
}