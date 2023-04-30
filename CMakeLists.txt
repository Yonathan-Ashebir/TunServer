cmake_minimum_required(VERSION 3.16)
project(TunServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(TunServer
        src/handler/Main.cpp
        src/connection/TCPConnection.cpp
        src/handler/Handler.cpp
        src/packet/Packet.cpp
        src/packet/TCPPacket.cpp
        src/tunnel/Tunnel.cpp)

add_executable(test src/Test.cpp)