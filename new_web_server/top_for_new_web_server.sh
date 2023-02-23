top -p `ps -ef|grep new_web_server|grep -v $0 |grep -v grep |grep -v "start_new_web_server.sh"|awk -F " " '{print $2}'`
