top -p `ps -ef|grep jt1078_server|grep -v $0 |grep -v grep |grep -v "start_jt1078_server.sh"|awk -F " " '{print $2}'`

