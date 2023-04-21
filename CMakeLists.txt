cmake_minimum_required(VERSION 3.16)
project(TunServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(TunServer src/Main.cpp src/HandleTun.cpp src/HandleTun.h src/Test.cpp src/Packet.h src/Include.h)

add_executable(test src/Test.cpp)