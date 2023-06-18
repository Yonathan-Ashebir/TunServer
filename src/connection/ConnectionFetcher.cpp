//
// Created by yoni_ash on 5/29/23.
//
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <regex>
#include <curl/curl.h>
#include <ArduinoJson.h>
#include "ConnectionFetcher.h"

using namespace std;

#define SERVER_REGISTER_FORMAT "action=register&id=%ud&name=%s&addr=%s&port=%d"

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

ConnectionFetcher::ConnectionFetcher(const string &serverName, const on_result_t &onRes) : onResult(onRes) {
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

void ConnectionFetcher::setOnResult(const ConnectionFetcher::on_result_t &onRes) {
    if (onRes == nullptr)throw invalid_argument("Null callback was supplied");
    onResult = onRes;
}

struct ResponseData {
    char *buf = new char[len];
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

void ConnectionFetcher::fetchConnections(const string &url) {
    DynamicJsonDocument doc{10000};
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (!curl) exitWithError("Could not generate curl's handle");
    ResponseData data{};
    char publicIp[INET6_ADDRSTRLEN]{};
    unsigned short publicPort{};
    char buf[100];

    auto updateAddress = [&, this] {
        auto publicAddress = getTcpMappedAddress(bindAddr);
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
        printf("My address = %s:%d\n", publicIp, publicPort);
#endif
        sleep(2);
    };
    updateAddress();

    auto updateUrl = [&, this] {
        snprintf(buf, sizeof buf, SERVER_REGISTER_FORMAT, id, curl_easy_escape(curl, serverName.c_str(), 0), publicIp,
                 publicPort);
        curl_easy_setopt(curl, CURLOPT_URL, (url + "?" + buf).c_str());
    };
    updateUrl();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
#ifdef LOGGING
    ::printf("Starting connection request fetching\n");
#endif
    while (shouldRun) {
        data.ind = 0;
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK) {
#ifdef LOGGING
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
#endif
        } else {
            /*Parse result*/
            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code != 200 && http_code != 302) {
#ifdef LOGGING
                fprintf(stderr, "Request failed: %ld\n",
                        http_code);
#endif

            } else {
                deserializeJson(doc, data.buf);
                if (doc.containsKey("id")) {
                    unsigned int tempId = doc["id"];
                    if (tempId != id) {
                        id = tempId;
#ifdef LOGGING
                        printf("Connected to relay server with an id of %d\n", id);
#endif
                        updateUrl();
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
                            if (inet_pton(AF_INET, conn["addr"], &addr.sin_addr.s_addr) == -1)continue;//todo: only ipv4

                            result.push_back(ResultItem{id, addr});
                        } catch (...) {}
                    }
#ifdef LOGGING
                    ::printf("Processed response and got %zu connections\n", result.size());
#endif
                    if (!result.empty())onResult(result);
                }
            }
        }


        updateAddress();
        usleep(5000000);
    }
    /* always cleanup */
#ifdef LOGGING
    ::printf("Finishing connection request fetching\n");
#endif
    curl_easy_cleanup(curl);
    started = false;
}

void ConnectionFetcher::setBindAddress(sockaddr_storage &addr) {
    bindAddr = addr;
}

sockaddr_storage ConnectionFetcher::getBindAddress() {
    return bindAddr;
}

unsigned int ConnectionFetcher::getId() const {
    return id;
}

#pragma clang diagnostic pop