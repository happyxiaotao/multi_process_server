cmake_minimum_required(VERSION 3.1)

project (jt1078_server)

set(CMAKE_CXX_STANDARD 14)

# 是否使用asan进行内存泄漏检测
#option(WITH_ASAN "Enable AddressSanitizer (ASan)" ON)
option(WITH_ASAN "Enable AddressSanitizer (ASan)" OFF)
#option(WITH_DEBUG "Enable DEBUG" ON)
option(WITH_DEBUG "Enable DEBUG" OFF)
if(WITH_ASAN)
	message("WITH_ASAN")
	add_definitions("-fsanitize=address -fno-omit-frame-pointer")
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

add_executable(jt1078_server
    ../core/log/Log.cpp
    ../core/eventloop/EventLoop.cpp
    ../core/socket/Socket.cpp
    ../core/tcp/Listener.cpp
    ../core/tcp/TcpSession.cpp
    ../core/buffer/Buffer.cpp
    ../core/time/TimeUtil.cpp
    ../core/database/RedisClient.cpp
    ../core/database/RedisMqServer.cpp
    ../core/timer/EventWatcher.cpp
    ../core/timer/TimerEventWatcher.cpp
    ../core/ipc_packet/IpcPktDecoder.cpp
    
    ../jt1078/forward_server/ForwardServer.cpp
    ../jt1078/forward_server/Channel.cpp
    ../jt1078/forward_server/Publisher.cpp
    ../jt1078/forward_server/Subscriber.cpp

    ../jt1078/jt1078_server/Jt1078Server.cpp
    ../jt1078/jt1078_server/CarManager.cpp
    ../jt1078/jt1078_server/CarSession.cpp
    ../jt1078/jt1078_server/Jt1078Decoder.cpp
    ../jt1078/Jt1078Util.cpp
    ../jt1078/Jt1078Service.cpp
    main.cpp
)

# 设置输出目录
set(JT1078_DIR ${CMAKE_SOURCE_DIR}/bin/jt1078_server)

# 设置执行程序输出目录
SET(EXECUTABLE_OUTPUT_PATH ${JT1078_DIR})

# 拷贝相关脚本
file(COPY top_for_jt1078_server.sh DESTINATION ${JT1078_DIR})
file(COPY start_jt1078_server.sh DESTINATION ${JT1078_DIR})
file(COPY stop_jt1078_server.sh DESTINATION ${JT1078_DIR})

# 拷贝配置文件
file(COPY ../conf/jt1078.ini DESTINATION ${JT1078_DIR}/conf)

# # 拷贝Dockerfile相关
# file(COPY Dockerfile DESTINATION ${JT1078_DIR})
# file(COPY DockerClear.sh DESTINATION ${JT1078_DIR})
# file(COPY DockerBuild.sh DESTINATION ${JT1078_DIR})
# 