//
// Created by yoni_ash on 4/29/23.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_HANDLER_H
#define TUNSERVER_HANDLER_H

#include "../session/TCPSession.h"

class Handler {
public:
    explicit Handler(Tunnel &tunnel);

    bool start();

    void stop();

private:
    Tunnel &tunnel;
    mutex mtx;
    vector<TCPSession *> connections;
    bool shouldRun = false;
    bool upStreamShuttingDown = false;
    bool downStreamShuttingDown = false;
    bool downStreamActive = false;
    bool upStreamActive = false;

    fd_set rcv{};
    fd_set snd{};
    fd_set err{};
    socket_t maxFd{};

    void handleUpStream();

    void handleDownStream();

    void cleanUp();
};


#endif //TUNSERVER_HANDLER_H

#pragma clang diagnostic pop