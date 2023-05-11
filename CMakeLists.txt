cmake_minimum_required(VERSION 3.16)
project(TunServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(TunServer
        src/handler/Main.cpp
        src/connection/TCPConnection.cpp
        src/tunnel/DatagramTunnel.cpp
        src/handler/Handler.cpp
        src/packet/TCPPacket.cpp
        src/error/Error.cpp)
if(WIN32)
    target_link_libraries(TunServer wsock32 ws2_32)
endif()
add_executable(test src/error/Error.cpp src/Test.cpp )
add_executable(listener src/Listener.c src/TCPRaw.c)