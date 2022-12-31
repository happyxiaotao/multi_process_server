sh stop_web_server.sh

chmod +x web_server

# 设置允许生成core文件
ulimit -c unlimited
sysctl -w kernel.core_uses_pid=1

# 添加ffmpeg依赖库
cur_dir=`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$cur_dir/run_lib

./web_server
