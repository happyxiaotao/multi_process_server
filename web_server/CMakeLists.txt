cmake_minimum_required(VERSION 3.1)

project (web_server)

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

link_libraries(pthread event.a event_pthreads.a event_openssl.a spdlog.a ssl crypto
                -lavcodec -lavutil -lavformat -lswresample -lswscale)

add_executable(web_server
    ../core/log/Log.cpp
    ../core/eventloop/EventLoop.cpp
    ../core/socket/Socket.cpp
    ../core/buffer/Buffer.cpp
    ../core/tcp/TcpSession.cpp
    ../core/tcp/TcpClient.cpp
    ../core/ipc_packet/IpcPktDecoder.cpp
    ../core/http_server/HttpHandler.cpp
    ../core/http_server/HttpServer.cpp
    ../core/timer/EventWatcher.cpp
    ../core/timer/TimerEventWatcher.cpp

    ../jt1078/Jt1078Util.cpp

    ../jt1078_client/Jt1078Client.cpp

    ./http/HttpSrsOnSessionHandler.cpp
    ./http/HttpWebOnPlayHandler.cpp
    ./http/WebServer.cpp
    ./http/DeviceIdMgr.cpp

    ./rtmp/RtmpThread.cpp

    ./rtmp/RtmpStream/RtmpStream.cpp
    ./rtmp/RtmpStream/RtmpClient.cpp
    ./rtmp/RtmpStream/RtmpMgr.cpp

    ./audio/Adpcm.cpp
    ./audio/CCodec.cpp
    ./audio/G711.cpp
    ./audio/g726.cpp

    main.cpp
)


# 设置输出目录
set(WEB_DIR ${CMAKE_SOURCE_DIR}/bin/web_server)

# 设置执行程序输出目录
SET(EXECUTABLE_OUTPUT_PATH ${WEB_DIR})

# 拷贝相关脚本
file(COPY top_for_web_server.sh DESTINATION ${WEB_DIR})
file(COPY start_web_server.sh DESTINATION ${WEB_DIR})
file(COPY stop_web_server.sh DESTINATION ${WEB_DIR})

# 拷贝配置文件
file(COPY ../conf/web.ini DESTINATION ${WEB_DIR}/conf)

# 拷贝依赖库
file(COPY run_lib DESTINATION ${WEB_DIR})

# 拷贝Dockerfile相关
file(COPY Dockerfile DESTINATION ${WEB_DIR})
file(COPY DockerClear.sh DESTINATION ${WEB_DIR})
file(COPY DockerBuild.sh DESTINATION ${WEB_DIR})


