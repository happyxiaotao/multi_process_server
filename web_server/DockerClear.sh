#! /bin/bash
$SERNAME="web_server"
ProcNumber=`docker inspect "$SERNAME"| grep "Error"`
echo "$ProcNumber"
if [ "$ProcNumber" = "Error: No such object: $SERNAME" ];then
   echo "continer is not run $SERNAME"
else
   echo "continer is running $SERNAME"
   data=`docker stop $SERNAME -f && docker rm $SERNAME -f && docker rmi $SERNAME:1.1`
   echo $data
   echo "清理成功"
fi
