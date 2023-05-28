cmake_minimum_required(VERSION 3.16)
project(TunServer)

add_subdirectory(external/ArduinoJson)

add_executable(jsonTest src/json/JsonTest.cpp)
##find_package(Nightly REQUIRED)
target_include_directories(jsonTest PRIVATE external/ArduinoJson/src/ArduinoJson external/ArduinoJson/src)

set(CMAKE_CXX_STANDARD 14)

add_executable(tunServer
        src/handler/Main.cpp
        src/connection/TCPConnection.cpp
        src/tunnel/DatagramTunnel.cpp
        src/handler/Handler.cpp
        src/packet/TCPPacket.cpp
        src/error/Error.cpp)
add_executable(test src/error/Error.cpp src/Test.cpp
        src/tunnel/DatagramTunnel.cpp
        src/packet/TCPPacket.cpp
        src/error/Error.cpp)
add_executable(listener src/Listener.c src/TCPRaw.c)
add_executable(handlerTest src/handler/HandlerTest.cpp
        src/connection/TCPConnection.cpp
        src/tunnel/DatagramTunnel.cpp
        src/packet/TCPPacket.cpp
        src/error/Error.cpp src/packet/CompressedTCPPacket.cpp src/packet/CompressedTCPPacket.h)
if (WIN32)
    target_link_libraries(tunServer wsock32 ws2_32)
    target_link_libraries(test wsock32 ws2_32)
    target_link_libraries(listener wsock32 ws2_32)
    target_link_libraries(handlerTest wsock32 ws2_32)
endif ()
