//
// Created by yoni_ash on 5/29/23.
//

#ifndef TUNSERVER_CONNECTIONFETCHER_H
#define TUNSERVER_CONNECTIONFETCHER_H

#include <string>
#include "../Include.h"

using namespace std;
struct ResultItem {
    unsigned int id;
    sockaddr_storage addr;
};

class ConnectionFetcher {
public:
    using on_result_t = std::function<void(vector<ResultItem>&)>;

    const
    char *NAME_PATTERN = "[_a-zA-Z0-9- ]*";

    ConnectionFetcher(ConnectionFetcher &) = delete;

    ConnectionFetcher(ConnectionFetcher &&) = delete;

    ConnectionFetcher &operator=(ConnectionFetcher &) = delete;

    ConnectionFetcher &operator=(ConnectionFetcher &&) = delete;

    explicit ConnectionFetcher(const string &serverName, const on_result_t& onRes);

    void setOnResult(const on_result_t& onRes);

    void setServerName(const string &name);

    string getServerName();

    void setBindAddress(sockaddr_storage &addr);

    [[nodiscard]] unsigned int getId() const;

    sockaddr_storage getBindAddress();

    void start(const string &url);

    void stop();

private:
    on_result_t onResult;
    string serverName{};
    sockaddr_storage bindAddr{};
    unsigned int id{};
    mutex mtx;
    bool started{false};
    bool shouldRun{false};

    void fetchConnections(const string &url);
};


#endif //TUNSERVER_CONNECTIONFETCHER_H
