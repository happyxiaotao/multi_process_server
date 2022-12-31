cmake_minimum_required(VERSION 3.1)

project (pc_server)

set(CMAKE_CXX_STANDARD 14)

# 是否使用asan进行内存泄漏检测
#option(WITH_ASAN "Enable AddressSanitizer (ASan)" ON)
option(WITH_ASAN "Enable AddressSanitizer (ASan)" OFF)
#option(WITH_DEBUG "Enable DEBUG" ON)
option(WITH_DEBUG "Enable DEBUG" OFF)
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

link_libraries(pthread event.a event_pthreads.a spdlog.a)

add_executable(pc_server
    ../core/log/Log.cpp
    ../core/eventloop/EventLoop.cpp
    ../core/socket/Socket.cpp
    ../core/tcp/Listener.cpp
    ../core/tcp/TcpSession.cpp
    ../core/tcp/TcpClient.cpp
    ../core/buffer/Buffer.cpp
    ../core/time/TimeUtil.cpp
    ../core/timer/EventWatcher.cpp
    ../core/timer/TimerEventWatcher.cpp
    ../core/ipc_packet/IpcPktDecoder.cpp
    
    ../jt1078/Jt1078Util.cpp
    PcServer.cpp
    PcSession.cpp
    PcManager.cpp
    PcPublisher.cpp
    ../jt1078_client/Jt1078Client.cpp
    main.cpp
)



# 设置输出目录
set(PC_DIR ${CMAKE_SOURCE_DIR}/bin/pc_server)

# 设置执行程序输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PC_DIR})

# 拷贝相关脚本
file(COPY top_for_pc_server.sh DESTINATION ${PC_DIR})
file(COPY start_pc_server.sh DESTINATION ${PC_DIR})
file(COPY stop_pc_server.sh DESTINATION ${PC_DIR})

# 拷贝配置文件
file(COPY ../conf/pc.ini DESTINATION ${PC_DIR}/conf)

# # 拷贝Dockerfile相关
# file(COPY Dockerfile DESTINATION ${PC_DIR})
# file(COPY DockerClear.sh DESTINATION ${PC_DIR})
# file(COPY DockerBuild.sh DESTINATION ${PC_DIR})
