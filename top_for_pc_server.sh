top -p `ps -ef|grep pc_server|grep -v $0 |grep -v grep |grep -v "start_pc_server.sh"|awk -F " " '{print $2}'`

