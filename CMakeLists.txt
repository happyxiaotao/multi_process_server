cmake_minimum_required(VERSION 3.1)

# 设置输出目录
set(BIN_DIR ${CMAKE_HOME_DIRECTORY}/bin)

# 拷贝配置文件到bin目录
# file(COPY conf DESTINATION .)
file(COPY conf DESTINATION ${BIN_DIR})

# 拷贝相关脚本文件到bin目录
file(COPY ./start_jt1078_server.sh DESTINATION ${BIN_DIR})
file(COPY ./start_pc_server.sh DESTINATION ${BIN_DIR})
file(COPY ./top_for_jt1078_server.sh DESTINATION ${BIN_DIR})
file(COPY ./top_for_pc_server.sh DESTINATION ${BIN_DIR})

# 设置执行程序输出目录
SET(EXECUTABLE_OUTPUT_PATH ${BIN_DIR})

add_subdirectory(jt1078)
add_subdirectory(pc_server)
add_subdirectory(srs_server)