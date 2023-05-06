//
// Created by yoni_ash on 4/29/23.
//

#ifndef TUNSERVER_HANDLER_H
#define TUNSERVER_HANDLER_H

#include "../connection/TCPConnection.h"

#define MAX_CONNECTION 200;

class Handler {
public:
    explicit Handler(Tunnel &tunnel);

    bool start();

    void stop();

private:
    Tunnel &tunnel;
    mutex mtx;
    unsigned int connectionsCount = 0;
    unsigned int connectionsSize = 20;
    TCPConnection **connections = new TCPConnection *[connectionsSize];
    bool shouldRun = false;
    bool upStreamShuttingDown = false;
    bool downStreamShuttingDown = false;

    fd_set rcv{};
    fd_set snd{};
    fd_set err{};
    int maxFd{};

    void handleUpStream();

    void handleDownStream();

    void cleanUp();
};


#endif //TUNSERVER_HANDLER_H
