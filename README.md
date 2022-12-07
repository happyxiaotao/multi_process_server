# multi_process_server
多进程服务器，包含多个进程，比如jt1078服务器、PC服务器，以及后续的SRS服务器

#### 目前第三方组件和端口使用情况
jt1078_server: 
    9501：汽车连接的端口
    9511: pc_server连接的端口（如果jt1078_server和pc_server在同一机器，则不需要对外开通）
    redis：连接redis向808服务器发送指令
pc_server: 
    9521：Qt客户端连接的端口