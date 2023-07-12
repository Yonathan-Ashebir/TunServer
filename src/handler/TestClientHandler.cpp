//
// Created by yoni_ash on 7/10/23.
//
#include "ClientHandler.h"

void testDeactivation() {
    ClientHandler handler{};
    handler.start(1);
}

int main() {

    testDeactivation();
    this_thread::sleep_for(chrono::hours(24));
}