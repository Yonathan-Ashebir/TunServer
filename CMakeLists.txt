cmake_minimum_required(VERSION 3.16)
project(TunServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(RUNTIME_OUTPUT_DIRECTORY build_windows)
set(COMMON_SOURCES)

add_subdirectory(external/ArduinoJson)
include_directories(external/ArduinoJson/src/ArduinoJson external/ArduinoJson/src)

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
include_directories(external/ArduinoJson/src/ArduinoJson external/ArduinoJson/src external/curl/include)

add_subdirectory(external/stun)
include_directories(external/stun/include)

# Executables
add_executable(tunServer src/handler/Main.cpp src/session/TCPSession.cpp src/handler/Handler.cpp src/packet/TCPPacket.cpp ${COMMON_SOURCES})

add_executable(test src/Test.cpp src/packet/TCPPacket.cpp src/Test2.cpp ${COMMON_SOURCES})
target_link_libraries(test PRIVATE stunmsg)

add_executable(listener src/Listener.c)

add_executable(handlerTest src/handler/HandlerTest.cpp
        src/session/TCPSession.cpp

        src/packet/TCPPacket.cpp
        ${COMMON_SOURCES} src/packet/TCPPacket.cpp)

add_executable(jsonTest src/json/JsonTest.cpp ${COMMON_SOURCES})

add_executable(curlTest src/CurlTest.cpp ${COMMON_SOURCES})
target_link_libraries(curlTest PRIVATE libcurl)

add_executable(gccTest src/GCCTest.cpp)

add_executable(connectionBuilder src/connection/ConnectionBuilderTest.cpp
        src/connection/ConnectionFetcher.cpp
        ${COMMON_SOURCES} src/helpers/net/Socket.h src/helpers/net/Utilities.h src/helpers/net/SocketImp.h src/helpers/net/Stun.h)
target_link_libraries(connectionBuilder PRIVATE libcurl stunmsg)

add_executable(stun src/stun/Stun.cpp ${COMMON_SOURCES})

add_executable(stunLib src/stun/StunLib.cpp ${COMMON_SOURCES})
target_link_libraries(stunLib PRIVATE stunmsg)

add_executable(tunClient src/connection/Client.cpp ${COMMON_SOURCES})
target_link_libraries(tunClient PRIVATE libcurl stunmsg)

add_executable(smartBuffer src/helpers/buffer/SmartBufferTest.cpp ${COMMON_SOURCES})

#Tests
#add_test(NAME ConnectionBuilderTest COMMAND connectionBuilder)

# Windows compatibility
if (WIN32)
    set(WIN32_NETWORK_LIBS wsock32 ws2_32)
    target_link_libraries(tunServer PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(connectionBuilder PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(curlTest PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(listener PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(handlerTest PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(stun PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(test PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(stunLib PRIVATE ${WIN32_NETWORK_LIBS})
    target_link_libraries(tunClient PRIVATE ${WIN32_NETWORK_LIBS})
else ()

endif ()
#some libs: ws2_32 wldap32 wsock32 bcrypt