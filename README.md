# multi_process_server
多进程服务器，包含多个进程，比如jt1078服务器、PC服务器，以及后续的SRS服务器

#### 目前第三方组件和端口使用情况

目前端口使用情况

| 服务          | 端口 | 说明                                            |
| ------------- | ---- | ----------------------------------------------- |
| jt1078_server | 9501 | 对外汽车连接                                    |
| jt1078_server | 9511 | 对内pc_server与web_server                       |
| redis         | 6379 | 看具体redis端口，目前jt1078_server会是会用redis |
| pc_server     | 9521 | 对外，Qt客户端连接，获取实时视频                |
| web_server    | 9531 | 对外，处理SRS回调。                             |

​	

### web_server

##### 提供的功能：

1. 提供音视频编解码功能，生成RTMP流，并推送到SRS
2. 提供Http服务器，支持SRS回调
3. 提供Http服务器，（与张总一起处理？，当浏览器播放某条视频时，向Http服务器发送消息，然后才播放。通过srs服务器，向jt1078服务器，订阅某个车辆数据）
4. 提供发送订阅功能，连接jt1078_server，订阅或取消订阅数据

##### 测试接口

外界通知订阅通道

```bash
curl "http://127.0.0.1:9531/web_on_play" -X POST -d  "{\"device_id\":\"06495940943803\"}"
```

SRS回调接口-开始播放

```bash
curl "http://127.0.0.1:9531/api/v1/sessions" -X POST -d "{\"client_id\":\"dsfsdfsafd\",\"stream\":\"06495940943803\",\"action\":\"on_play\"}"
```

SRS回调接口-关闭窗口

```bash
curl "http://127.0.0.1:9531/api/v1/sessions" -X POST -d "{\"client_id\":\"dsfsdfsafd\",\"stream\":\"06495940943803\",\"action\":\"on_stop\"}"
```

SRS网页端播放连接

```bash
http://127.0.0.1:8080/live/06495940943803.flv
```



#### SRS相关

##### docker中srs的目录

```
/usr/local/srs
```

##### srs的http回调配置

回调配置项在http_hooks中。实时和历史需要各开一个。

```
vhost __defaultVhost__ {
    hls {
        enabled         on;
    }
    http_remux {
        enabled     on;
        mount       [vhost]/[app]/[stream].flv;
    }
    http_hooks {
    	enabled         on;
    	# 网页打开一个视频，会回调此接口
 		on_play			http://127.0.0.1:9992/api/v1/sessions;
        #关闭网页或者其他原因导致无法继续播放，会回调此接口
        on_stop			http://127.0.0.1:9992/api/v1/sessions;
    }
}

```

#### web_server与SRS的交互

通过在SRS的配置文件中配置`http_hooks`选项。

当回调`on_play`时，`web_server`向`jt1078_server`发送订阅请求。

当回调`on_stop`时，`web_server`向`jt1078_server`发送取消订阅请求。

`web_server`针对SRS的回调中不同的`client_id`有着统一处理，保证同一个通道，只会发送一次订阅/取消订阅请求，不会出现重复订阅/取消订阅的情况






##### 对外暴露的端口：

目前只需要对外暴露一个Http端口即可。

1. 处理SRS回调
2. 处理张总的播放请求



##### 连接数：

1. 与jt1078_server的连接
2. 与SRS的RTMP连接（由ffmpeg自动创建）



内部实现：
1、Http服务器：提供SRS回调处理（播放、停止）
2、Jt1078Client: 向jt1078_server发送消息，获取车辆数据
3、多线程（音视频处理）





### Docker部署

##### 创建docker容器



#### web_server

注意事项：

1. 对外暴露的端口号：9531
2. 是否需要配置文件：是
3. 是否需要日志文件：是
4. 