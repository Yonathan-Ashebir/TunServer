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

string validateServerName(string &name) {
    smatch matches{};
    regex pat(R"([-_a-zA-Z0-9- ]*)");
    if (regex_search(name, matches, pat)) {
        return matches[0];
    } else throw invalid_argument("Bad server name supplied");
}

void ConnectionFetcher::start() {
    unique_lock<mutex> lock(data->mtx);
    if (!data->started) {
        data->started = true;
        data->shouldRun = true;
        lock.unlock();
        thread th{[&] { fetchConnections(); }};
        th.detach();
    }
}

ConnectionFetcher::ConnectionFetcher(string serverName, string serverURL, OnConnectionRequest onRes) : data(
        new Data(std::move(validateServerName(serverName)), std::move(serverURL), std::move(onRes))) {
}

string ConnectionFetcher::getServerName() {
    return data->serverName;
}

void ConnectionFetcher::stop() {
    data->shouldRun = false;
}

void ConnectionFetcher::setOnConnectionRequest(const OnConnectionRequest &onReq) {
    if (!onReq)throw invalid_argument("Empty callback is disallowed");
    data->onRequest = onReq;
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

void ConnectionFetcher::fetchConnections() {
    DynamicJsonDocument doc{1000};
    chrono::steady_clock::time_point lastTimeUpdatedAddress{};
    constexpr static chrono::milliseconds STUN_TIMEOUT{3000};
    constexpr static chrono::seconds STUN_INTERVAL{5};

    auto updateURL = [&] {
        curl_easy_setopt(data->curl, CURLOPT_URL,
                         (data->serverURL + "?action=register&id=" + to_string(data->id) + "&addr=" +
                          getIPString(data->publicAddr) + "&port=" + to_string(getPort(data->publicAddr))).c_str());

    };

    auto updateAddress = [&, this]() -> bool {
        try {
            auto res = getTCPPublicAddress(data->bindAddr, STUN_TIMEOUT);
            lastTimeUpdatedAddress = chrono::steady_clock::now();
#ifdef LOGGING
            cout << "My address = " << getAddressString(*res) << endl;
#endif

            if (*res != data->publicAddr) {
                data->publicAddr = *res;
                updateURL();
            }
            return true;
        } catch (exception &e) {
#ifdef LOGGING
            cerr << "Updating address failed\n   what(): " << e.what() << endl;
#endif
            return false;
        }
    };

    curl_easy_setopt(data->curl, CURLOPT_WRITEFUNCTION, reader);
    ResponseData responseData{};
    curl_easy_setopt(data->curl, CURLOPT_WRITEDATA, &responseData);

#ifdef LOGGING
    cout << "Starting connection request fetching" << endl;
#endif
    while (data->shouldRun) {
        auto now = chrono::steady_clock::now();
        if (now - lastTimeUpdatedAddress >= STUN_INTERVAL && !updateAddress()) {
            this_thread::sleep_for(chrono::milliseconds(100));
            if (data->retryCount == data->maxRetries) {
                data->retryCount = 0;
                stop();
                data->onError();
            } else
                data->retryCount++;
            continue;
        }

        responseData.ind = 0;
        CURLcode res = curl_easy_perform(data->curl);

        /* check for errors */
        long httpCode{};
        if (res != CURLE_OK || (curl_easy_getinfo (data->curl, CURLINFO_RESPONSE_CODE, &httpCode), httpCode != 200)) {
#ifdef LOGGING
            if (res != CURLE_OK)
                cerr << "curl_easy_perform() failed: " <<
                     curl_easy_strerror(res) << endl;
            else cerr << "Request failed: " << httpCode << endl;
#endif
            if (data->retryCount == data->maxRetries) {
                data->retryCount = 0;
                stop();
                data->onError();
            } else
                data->retryCount++;
            continue;
        }

        /*parse result*/
        deserializeJson(doc, responseData.buf);
        if (doc.containsKey("id")) {
            unsigned int tempId = doc["id"];
            if (tempId != data->id) {
                data->id = tempId;
#ifdef LOGGING
                cout << "Connected to relay server with an id of " << data->id << endl;
#endif
                updateURL();
            }
        }

        if (doc.containsKey("connections")) {
            vector<ConnectRequest> result;
            auto conns = doc["connections"];
            for (int i = 0; i < conns.size(); i++) {
                auto conn = conns[i];
                try {
                    long clientId = conn["client_id"];
                    if (clientId < 1)continue;
                    string addrName = conn["addr"];
                    int port = conn["port"];
                    if (port < 1 || port > 65535)continue;
                    sockaddr_storage addr{};
                    initialize(addr, AF_INET, addrName.c_str(), port);
                    //todo: only ipv4
                    result.push_back(ConnectRequest{data->id, addr});
                } catch (...) {}
            }
#ifdef LOGGING
            cout << "Processed response and got connection of count: " << result.size() << endl;
#endif
            if (!result.empty())data->onRequest(result);
        }


        data->retryCount = 0;
        this_thread::sleep_for(data->fetchInterval);
    }

#ifdef LOGGING
    cout << "Finishing connection request fetching" << endl;
#endif
    data->started = false;
}

void ConnectionFetcher::setBindAddress(sockaddr_storage &addr) {
    data->bindAddr = addr;
}

sockaddr_storage &ConnectionFetcher::getBindAddress() {
    return data->bindAddr;
}

unsigned int ConnectionFetcher::getId() const {
    return data->id;
}

void ConnectionFetcher::setOnConnectionRequest(const ConnectionFetcher::OnConnectionRequest &&onReq) {
    setOnConnectionRequest(onReq);
}

#pragma clang diagnostic pop