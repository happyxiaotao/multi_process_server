#ifndef PIPE_PKT_TYPE_H
#define PIPE_PKT_TYPE_H
#include <stdint.h>
enum PipePktType : uint32_t
{
    PIPE_PKT_TYPE_INVALID = 0,

    PIPE_PKT_STOP_THREAD, // 主线程通知子线程停止

    PIPE_PKT_NEW_CAR_CLIENT,
    PIPE_PKT_UPLOAD_JT1078_ICCID, // 添加新的iccid，在获取到数据包之后，解析得到iccid后发送到主线程保存
    PIPE_PKT_KICK_ICCID,          // iccid互踢

    PIPE_PKT_FORWARD_JT1078, // 转发jt1078包
                             // pipe_header
                             // pipe_body
                             //      iccid(uint64_t)类型
                             //      pipe_other_data

    PIPE_PKT_WEB_ON_STOP,     // 发送通知srs回调了on_stop，并且没有其他人在观看
                              // web -> http ： on_stop
                              // http -> main : PIPE_PKT_WEB_ON_STOP
                              // main -> jt1078: PIPE_PKT_WEB_ON_STOP
                              // main -> srs: PIPE_PKT_WEB_ON_STOP
    PIPE_PKT_PUSH_SRS_FAILED, //推送数据流到srs失败


    PIPE_PKT_NEW_PC_CLIENT, // 新的PC客户端连接
    PIPE_PKT_UPLOAD_PC_ICCID,//添加pc想要的iccid数据
};

#endif // PIPE_PKT_TYPE_H