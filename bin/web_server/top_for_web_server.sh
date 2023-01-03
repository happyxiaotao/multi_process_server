top -p `ps -ef|grep web_server|grep -v $0 |grep -v grep |grep -v "start_pc_server.sh"|awk -F " " '{print $2}'`

