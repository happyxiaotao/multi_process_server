# multi_process_server
多进程服务器，包含多个进程，比如jt1078服务器、PC服务器，以及后续的SRS服务器

#### 目前第三方组件和端口使用情况
jt1078_server: 
    9501：汽车连接的端口
    9511: pc_server连接的端口（如果jt1078_server和pc_server在同一机器，则不需要对外开通）
    redis：连接redis向808服务器发送指令
pc_server: 
    9521：Qt客户端连接的端口

srs_server:
    9531: Http端口，用来SRS回调。

​	





#### srs_server

连接JT1078服务器，获取车辆数据
连接SRS，推送Rtmp数据。
浏览器从SRS获取视频流数据。

内部实现：
1、Http服务器：提供SRS回调处理（播放、停止）
2、Jt1078Client: 向jt1078_server发送消息，获取车辆数据
3、多线程（音视频处理）