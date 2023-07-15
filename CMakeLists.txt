cmake_minimum_required(VERSION 3.16)
project(TunServer)

## standards ##
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
#if (MSVC)
#    add_compile_options(/W4 /WX)
#else()
#    add_compile_options(-Wall#[[ -Wextra -pedantic -Werror]])
#endif()

## includes ##
include(CTest)
include(FetchContent)
include(Utilities.cmake)

## externals ##
add_subdirectory(external/ArduinoJson)
#include_directories(external/ArduinoJson/src)

#set(HTTP_ONLY ON)
#set(BUILD_CURL_EXE OFF)
#set(CURL_STATICLIB ON)
#set(HB_STATIC_CURL ON)
#set(CMAKE_USE_LIBSSH2 ON)
#add_definitions(-DCURL_STATICLIB)
#option(BUILD_SHARED_LIBS "like set but only for boolean but useless if variable is already set" OFF)
# no = off = false = FaLse
#set(CURL_USE_WOLFSSL OFF)
#set(CURL_USE_OPENSSL OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(external/curl)
include_directories(external/curl/include)

add_subdirectory(external/ylib-cxx)
link_libraries(ylib-cxx)

add_subdirectory(external/tun-commons)
link_libraries(tun-commons)
#include_directories(external/tun-commons/include)


## targets ##
set(COMMON_SOURCES)

add_executable(tunServer src/handler/Main.cpp src/session/TCPSession.cpp src/handler/Handler.cpp ${COMMON_SOURCES})

add_executable(myTest src/Test.cpp src/Test2.cpp ${COMMON_SOURCES})

add_executable(listener src/Listener.c)

add_executable(handlerTest src/handler/HandlerTest.cpp
        src/session/TCPSession.cpp
        ${COMMON_SOURCES})

add_executable(jsonTest src/json/JsonTest.cpp ${COMMON_SOURCES})
target_link_libraries(jsonTest PRIVATE ArduinoJson)

add_executable(curlTest src/CurlTest.cpp ${COMMON_SOURCES})
target_link_libraries(curlTest PRIVATE libcurl)

#add_executable(gccTest src/GCCTest.cpp)

add_executable(connectionBuilder src/connection/ConnectionBuilderTest.cpp
        src/connection/ConnectionFetcher.cpp
        ${COMMON_SOURCES})
target_link_libraries(connectionBuilder PRIVATE libcurl ArduinoJson)


add_executable(stun src/stun/Stun.cpp ${COMMON_SOURCES})

add_executable(stunLib src/stun/StunLib.cpp ${COMMON_SOURCES})

add_executable(tunClient src/connection/Client.cpp ${COMMON_SOURCES})
target_link_libraries(tunClient PRIVATE libcurl ArduinoJson)

add_executable(TestClientHandler src/handler/TestClientHandler.cpp src/handler/ClientHandler.cpp ${COMMON_SOURCES})
add_test(NAME TestClientHandler COMMAND TestClientHandler)

#[[
set (passRegex "^Test passed" "^All ok")
set (failRegex "Error" "Fail")

set_property (TEST outputTest
        PROPERTY PASS_REGULAR_EXPRESSION "${passRegex}")
set_property (TEST outputTest
        PROPERTY FAIL_REGULAR_EXPRESSION "${failRegex}")]]


#Tests
#add_test(NAME ConnectionBuilderTest COMMAND connectionBuilder)

#some libs: ws2_32 wldap32 wsock32 bcrypt