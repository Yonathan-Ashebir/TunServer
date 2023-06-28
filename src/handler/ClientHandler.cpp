//
// Created by yoni_ash on 6/25/23.
//

#include "ClientHandler.h"

ClientHandler::ClientHandler(unsigned int id, OnTunnelRequest &callback) : clientId(id), requestCallback(callback) {

}

ClientHandler::ClientHandler(unsigned int id) : clientId(id), requestCallback() {

}
