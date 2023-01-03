chmod +x jt1078_server
cur_dir=`pwd`
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`
#echo $LD_LIBRARY_PATH

# 设置允许生成core文件
ulimit -c unlimited
sysctl -w kernel.core_uses_pid=1

./jt1078_server
