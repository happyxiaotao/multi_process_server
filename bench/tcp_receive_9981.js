/**
 * 构建TCP客户端
 */

/* 引入net模块 */
var net = require("net");

/* 创建TCP客户端 */
var client = net.Socket();

/* 设置连接的服务器 */
client.connect(9981, '127.0.0.1', function () {
    console.log("connect the server");

    /* 向服务器发送数据 */
    client.write("subscriber01234500055502");
})

/* 监听服务器传来的data数据 */
client.on("data", function (data) {
    // console.log("the data of server is " + data.toString());
    console.log("get data from server, data len:" + data.length);
})

/* 监听end事件 */
client.on("end", function () {
    console.log("data end");
})