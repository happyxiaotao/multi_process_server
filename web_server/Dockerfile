FROM ubuntu:18.04

# 对外暴露的端口
EXPOSE 9531

# 创建相关目录
RUN mkdir -p /web_server

# 进入/web_server目录
WORKDIR /web_server

# 拷贝执行程序
COPY web_server .

# 创建conf目录
RUN mkdir -p /web_server/conf
# 拷贝配置文件
COPY conf/web.ini /web_server/conf/web.ini

# 创建日志目录
RUN mkdir -p /web_server/logs

# 拷贝启动脚本
COPY start_web_server.sh .

# 添加相关依赖库
RUN apt update && apt upgrade
# 添加openssl依赖库
RUN apt install openssl -y
RUN apt autoremove -y

ENTRYPOINT [ "sh", "-c", "./start_web_server.sh"] 