//
// Created by yoni_ash on 6/25/23.
//

#ifndef TUNSERVER_CLIENTHANDLER_H
#define TUNSERVER_CLIENTHANDLER_H

#include "../Include.h"
#include "../tunnel/DisposableTunnel.h"

class ClientHandler {
public:
    using OnTunnelRequest = function<void(sockaddr_storage &addr, unsigned int clientId)>;

    ClientHandler(unsigned int id, OnTunnelRequest &callback);

    explicit ClientHandler(unsigned int id);

    bool isActive();

    void reset();

    void addTunnel(DisposableTunnel &tunnel);

    void setOnTunnelRequest(OnTunnelRequest callback);

    OnTunnelRequest getOnTunnelRequest();

private:
    unsigned int clientId;
    OnTunnelRequest requestCallback;
    bool shouldStop = true;
    bool started = false;

    void handleUpStream();
};


#endif //TUNSERVER_CLIENTHANDLER_H
