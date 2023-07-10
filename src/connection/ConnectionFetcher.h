//
// Created by yoni_ash on 5/29/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_CONNECTIONFETCHER_H
#define TUNSERVER_CONNECTIONFETCHER_H

#include <string>
#include "../Include.h"

using namespace std;
struct ConnectRequest {
    unsigned int id;
    sockaddr_storage addr;
};

class ConnectionFetcher {
public:
    using OnConnectionRequest = std::function<void(vector<ConnectRequest> &)>;

    constexpr static unsigned int MAX_RETRY = 5;
    constexpr static auto NAME_PATTERN = "[_a-zA-Z0-9- ]*";
    constexpr static chrono::seconds DEFAULT_FETCH_INTERVAL{3};
    constexpr static chrono::seconds MIN_FETCH_INTERVAL{1};


    explicit ConnectionFetcher(string serverName, string serverURL, OnConnectionRequest onRes);

    ConnectionFetcher(ConnectionFetcher &) = default;

    ConnectionFetcher(ConnectionFetcher &&) = default;

    ConnectionFetcher &operator=(const ConnectionFetcher &) = default;

    ConnectionFetcher &operator=(ConnectionFetcher &&) = default;

    void setOnConnectionRequest(const OnConnectionRequest &&onReq);

    void setOnConnectionRequest(const OnConnectionRequest &onReq);

    OnConnectionRequest &getOnConnectionRequest() {
        return data->onRequest;
    }

    void setOnError(function<void()> onError) {
        if (!onError)throw invalid_argument("Empty callback is disallowed");
        data->onError = std::move(onError);
    }

    function<void()> &getOnError() {
        return data->onError;
    }

    void setFetchInterval(chrono::seconds interval) {
        data->fetchInterval = max(interval, MIN_FETCH_INTERVAL);
    }

    chrono::seconds getFetchInterval() {
        return data->fetchInterval;
    }

    string getServerName();

    [[nodiscard]] unsigned int getId() const;

    void setBindAddress(sockaddr_storage &addr);

    sockaddr_storage &getBindAddress();

    sockaddr_storage &getPublicAddress() {
        return data->publicAddr;
    }

    void setMaximumRetries(unsigned int count) {
        data->maxRetries = count;
    }

    unsigned int getMaximumRetries() {
        return data->maxRetries;
    }

    void start();

    void stop();

    [[nodiscard]] bool isActive() const {
        return data->started;
    }

private:
    struct Data {
        string serverName;
        string serverURL;
        unsigned int id{};
        OnConnectionRequest onRequest;
        function<void()> onError{[] {}};

        sockaddr_storage bindAddr{};
        sockaddr_storage publicAddr{};

        CURL *curl{};

        bool started{false};
        bool shouldRun{false};
        chrono::seconds fetchInterval{DEFAULT_FETCH_INTERVAL};
        unsigned int retryCount{};
        unsigned int maxRetries{MAX_RETRY};
        mutex mtx{};

        Data(string &&name, string &&serverURL, OnConnectionRequest &&onRequest) : serverName(name),
                                                                                   serverURL(serverURL),
                                                                                   onRequest(onRequest) {
            if (!this->onRequest)throw invalid_argument("Empty callback is disallowed");
            curl = curl_easy_init();
            if (!curl) throw BadException("Could not initialize a 'curl' handle");
        }

        ~Data() {
            if (curl)curl_easy_cleanup(curl);
        }
    };

    shared_ptr<Data> data;


    void fetchConnections();
};


#endif //TUNSERVER_CONNECTIONFETCHER_H

#pragma clang diagnostic pop