//
// Created by yoni_ash on 5/29/23.
//

#include <regex>
#include <curl/curl.h>
#include <ArduinoJson.h>
#include "ConnectionFetcher.h"

void ConnectionFetcher::start(const string &url) {
    unique_lock<mutex> lock(mtx);
    if (!started) {
        started = true;
        shouldRun = true;
        lock.unlock();
        thread th{[this, url] { fetchConnections(url); }};
        th.join();
    }
}

ConnectionFetcher::ConnectionFetcher(const string &serverName, on_result_t onRes) : onResult(onRes) {
    if (onRes == nullptr)throw invalid_argument("Null callback was supplied");
    setServerName(serverName);
}

void ConnectionFetcher::setServerName(const string &name) {
    smatch matches;
    regex pat(R"([-_a-zA-Z0-9- ]*)");
    if (regex_search(name, matches, pat)) {
        serverName = matches[0];
    } else throw invalid_argument("Bad server name supplied");
}

string ConnectionFetcher::getServerName() {
    return serverName;
}

void ConnectionFetcher::stop() {
    shouldRun = false;
}

void ConnectionFetcher::setOnResult(ConnectionFetcher::on_result_t onRes) {
    if (onRes == nullptr)throw invalid_argument("Null callback was supplied");
    onResult = onRes;
}

struct ResponseData {
    char *buf = new char[len];
    size_t len{1000};
    size_t ind{};
};

size_t reader(char *buf, size_t _, size_t len, ResponseData *responseData) {
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

curl_socket_t socketGenerator(sockaddr_in *bindAddr, curlsocktype purpose, curl_sockaddr *addr) {
    curl_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    char val = 1;
#else
    int val = 1;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val) < -1)
        exitWithError("Could not set reuse address on curl socket");
    if (bindAddr != nullptr && bindAddr->sin_port != 0) {
        if (bind(sock, reinterpret_cast<const sockaddr *>(bindAddr), sizeof(sockaddr_in)) < 0)
            exitWithError("Could not bind socket for curl");
    }
    return sock;
}

void ConnectionFetcher::fetchConnections(const string &url) {
    DynamicJsonDocument doc{10000};
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (!curl) exitWithError("Could not generate curl's handle");
    ResponseData data{};
    curl_easy_setopt(curl, CURLOPT_URL, (url + "?action=register&id=" + to_string(id) + "&name=" +
                                         curl_easy_escape(curl, serverName.c_str(), 0)).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, socketGenerator);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &bindAddr);

#ifdef LOGGING
    ::printf("Starting connection request fetching\n");
#endif
    while (shouldRun) {
        data.ind = 0;
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        else {
            /*Parse result*/
            deserializeJson(doc, data.buf);
            if (doc.containsKey("id")) {
                unsigned int tempId = doc["id"];
                if (tempId != id) {
                    id = tempId;
#ifdef LOGGING
                    printf("Connected to relay server with an id of %d\n", id);
#endif
                    curl_easy_setopt(curl, CURLOPT_URL, (url + "?action=register&id=" + to_string(id) + "&name=" +
                                                         curl_easy_escape(curl, serverName.c_str(), 0)).c_str());
                }
            }

            if (doc.containsKey("connections")) {
                vector<ResultItem> result;
                auto conns = doc["connections"];
                for (int i = 0; i < conns.size(); i++) {
                    auto conn = conns[i];
                    try {
                        long clientId = conn["client_id"];
                        if (clientId < 1)continue;
                        sockaddr_in addr{};
                        addr.sin_family = AF_INET;
                        int port = conn["port"];
                        if (port < 1 || port > 65535)continue;
                        addr.sin_port = htons(port);
                        string addrName = conn["addr"];
                        if (inet_pton(AF_INET, conn["addr"], &addr.sin_addr.s_addr) < 0)continue;

                        result.push_back(ResultItem{id, addr});
                    } catch (...) {}
                }
#ifdef LOGGING
                ::printf("Processed response and got %zu connections\n", result.size());
#endif
                if (!result.empty())onResult(result);
            }
        }

        usleep(1000000);
    }
    /* always cleanup */
#ifdef LOGGING
    ::printf("Finishing connection request fetching\n");
#endif
    curl_easy_cleanup(curl);
    started = false;
}

void ConnectionFetcher::setBindAddress(sockaddr_in &addr) {
    bindAddr = addr;
}

sockaddr_in ConnectionFetcher::getBindAddress() {
    return bindAddr;
}

unsigned int ConnectionFetcher::getId() const {
    return id;
}
