cmake_minimum_required(VERSION 3.1)

set (ProgName "new_web_server")
project (${ProgName})

set(CMAKE_CXX_STANDARD 14)

# 是否使用asan进行内存泄漏检测
#option(WITH_ASAN "Enable AddressSanitizer (ASan)" ON)
option(WITH_ASAN "Enable AddressSanitizer (ASan)" OFF)
option(WITH_DEBUG "Enable DEBUG" ON)
#option(WITH_DEBUG "Enable DEBUG" OFF)
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

add_definitions("-Wall -std=c++14 -g -DASIO_STANDALONE")

set(INC_DIR /usr/include /usr/local/include . ../3part ../3part/ini  ../3part/spdlog)
set(LINK_DIR /usr/lib /usr/local/lib ../lib)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})
link_libraries(pthread  spdlog.a)

add_executable(${ProgName}
	../core/log/Log.cpp
	../core/buffer/Buffer.cpp
	../core/ipc_packet/IpcPktDecoder.cpp
	../jt1078/Jt1078Util.cpp
    CarTcpClient.cpp	
	CarWebSocketServer.cpp
	CarWebServer.cpp
    main.cpp
)


# # 设置执行程序输出目录
# 
# 拷贝相关脚本
file(COPY top_for_new_web_server.sh DESTINATION .)
file(COPY start_new_web_server.sh DESTINATION .)
file(COPY stop_new_web_server.sh DESTINATION .)
# 
# 拷贝配置文件夹
set(CONF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/conf)
# 拷贝 src文件夹下的内容  到  save_src_dir 中 
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/conf ${CONF_DIR})
# file(COPY ../conf/new_web_server.ini DESTINATION conf/new_web_server.ini)
# 
# # # 拷贝Dockerfile相关
# # file(COPY Dockerfile DESTINATION ${PC_DIR})
# # file(COPY DockerClear.sh DESTINATION ${PC_DIR})
# # file(COPY DockerBuild.sh DESTINATION ${PC_DIR})
# 