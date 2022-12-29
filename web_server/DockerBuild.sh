#! /bin/bash

# 此脚本用来自动编译docker镜像的

SERNAME="web_server"

data=`docker build -f ./Dockerfile -t $SERNAME:1.1 . && docker run -d -p 9531:9531 --name $SERNAME $SERNAME:1.1`
echo $data
echo "SUCCESS"
