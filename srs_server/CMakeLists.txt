cmake_minimum_required(VERSION 3.1)

project (srs_server)

set(CMAKE_CXX_STANDARD 14)

# 是否使用asan进行内存泄漏检测
#option(WITH_ASAN "Enable AddressSanitizer (ASan)" ON)
option(WITH_ASAN "Enable AddressSanitizer (ASan)" OFF)
option(WITH_DEBUG "Enable DEBUG" ON)
if(WITH_ASAN)
	message("WITH_ASAN")
	add_definitions("-fsanitize=address -fno-omit-frame-pointer")
    #-fsanitize=address 是开启内存越界检测
    #-fno-omit-frame-pointer 是去使能栈溢出保护
    #-static-libasan 是静态链接asan
	link_libraries("asan")
endif()
if(WITH_DEBUG)
	message("-O0")
	add_definitions("-O0")
else()
	message("-O2")
	add_definitions("-O2")
endif()

add_definitions("-Wall -std=c++14 -g")

set(INC_DIR /usr/include /usr/local/include . ../3part ../3part/ini  ../3part/spdlog)
set(LINK_DIR /usr/lib /usr/local/lib ../lib)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})

link_libraries(pthread event.a event_pthreads.a event_openssl.a spdlog.a hiredis.a)

add_executable(srs_server
    ../core/log/Log.cpp
    ../core/eventloop/EventLoop.cpp
    ../core/socket/Socket.cpp
    ../core/tcp/Listener.cpp
    ../core/tcp/TcpSession.cpp
    ../core/tcp/TcpClient.cpp
    ../core/buffer/Buffer.cpp
    ../core/time/TimeUtil.cpp
    ../core/database/RedisClient.cpp
    ../core/database/RedisMqServer.cpp
    ../core/timer/EventWatcher.cpp
    ../core/timer/TimerEventWatcher.cpp
    ../core/ipc_packet/IpcPktDecoder.cpp
    
    ../jt1078/jt1078_server/Jt1078Util.cpp
    PcServer.cpp
    PcSession.cpp
    PcManager.cpp
    PcPublisher.cpp
    Jt1078Client.cpp
    main.cpp
)
