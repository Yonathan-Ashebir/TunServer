cmake_minimum_required(VERSION 3.16)
project(TunServer)

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
include_directories(src/Include.h)
set(COMMON_SOURCES src/error/Error.cpp)

add_subdirectory(external/ArduinoJson)
include_directories(external/ArduinoJson/src/ArduinoJson external/ArduinoJson/src)

#set(HTTP_ONLY OFF)
#set(CMAKE_USE_OPENSSL ON)
#set(BUILD_CURL_EXE OFF)
#set(CURL_STATICLIB ON)
#set(HB_STATIC_CURL ON)
#set(CMAKE_USE_LIBSSH2 ON)
#add_definitions(-DCURL_STATICLIB)
#option(BUILD_SHARED_LIBS "like set but only for boolean but useless if variable is already set" OFF)
# no = off = false = FaLse
set(BUILD_SHARED_LIBS OFF)
add_subdirectory( external/curl)
include_directories(external/ArduinoJson/src/ArduinoJson external/ArduinoJson/src external/curl/include)

add_subdirectory(external/stun)
include_directories(external/stun/include)

# Executables
add_executable(tunServer  src/handler/Main.cpp src/session/TCPSession.cpp src/tunnel/DatagramTunnel.cpp src/handler/Handler.cpp src/packet/TCPPacket.cpp ${COMMON_SOURCES})

add_executable(test src/Test.cpp src/tunnel/DatagramTunnel.cpp src/packet/TCPPacket.cpp )

add_executable(listener src/Listener.c)

add_executable(handlerTest src/handler/HandlerTest.cpp
        src/session/TCPSession.cpp
        src/tunnel/DatagramTunnel.cpp
        src/packet/TCPPacket.cpp
        ${COMMON_SOURCES} src/packet/CompressedTCPPacket.cpp src/packet/CompressedTCPPacket.h)

add_executable(jsonTest src/json/JsonTest.cpp ${COMMON_SOURCES} )

add_executable(curlTest src/CurlTest.cpp ${COMMON_SOURCES} )

add_executable(connectionBuilder src/connection/ConnectionBuilderTest.cpp
        src/connection/ConnectionFetcher.cpp
        ${COMMON_SOURCES})

add_executable(stun src/stun/Stun.cpp  ${COMMON_SOURCES})

add_executable(stunLib src/stun/StunLib.cpp ${COMMON_SOURCES})

#Tests
#add_test(NAME ConnectionBuilderTest COMMAND connectionBuilder)

# Windows compatibility
if (WIN32)
    target_link_libraries(tunServer PRIVATE wsock32 ws2_32)
    target_link_libraries(connectionBuilder PRIVATE libcurl wsock32 ws2_32)
    target_link_libraries(curlTest PRIVATE libcurl ws2_32 wldap32 )
    target_link_libraries(test PRIVATE wsock32 ws2_32)
    target_link_libraries(listener PRIVATE wsock32 ws2_32)
    target_link_libraries(handlerTest PRIVATE wsock32 ws2_32)
    target_link_libraries(stun PRIVATE wsock32 ws2_32)
    target_link_libraries(stunLib PRIVATE stunmsg wsock32 ws2_32)
else ()
    target_link_libraries(curlTest PRIVATE libcurl)
    target_link_libraries(connectionBuilder PRIVATE libcurl)
    target_link_libraries(stunLib PRIVATE stunmsg)
endif ()
#some libs: ws2_32 wldap32 wsock32 bcrypt