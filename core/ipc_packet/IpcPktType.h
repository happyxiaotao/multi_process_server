#ifndef IPC_PACKET_TYPE_H
#define IPC_PACKET_TYPE_H
#include <stdint.h>

//
// 注意：如果修改此文件中各个变量标识的含义，需要同步修改Qt客户端IpcPktType.h文件，保持两端一致。否则会出现异常情况
//

namespace ipc
{
    const uint32_t IPC_PKT_INVALID_PACKET = 0; // 非法的包，不带任何信息

    // 数据包的类型，占低8位（21~32位）。 [0x00000001,0x000000ff]
    const uint32_t IPC_PKT_TYPE_MASK = 0x00000ff;
    const uint32_t IPC_PKT_TYPE_HEARTBEAT = 0x00000001;             // 心跳包
    const uint32_t IPC_PKT_TYPE_SUBSCRIBE_DEVICE_ID = 0x00000002;   // 订阅device_id
    const uint32_t IPC_PKT_TYPE_UNSUBSCRIBE_DEVICE_ID = 0x00000003; // 取消订阅device_id
    const uint32_t IPC_PKT_TYPE_JT1078_PACKET = 0x00000004;         // jt1078数据包

    // 数据包来源标识，占13~16位。[0x00010000,0x000f0000]
    const uint32_t IPC_PKT_FROM_MASK = 0x000f0000;
    const uint32_t IPC_PKT_FROM_JT1078_SERVER = 0x00010000; // 数据来源于jt1078_server
    const uint32_t IPC_PKT_FROM_WEB_SERVER = 0x00020000;    // 数据包来源于web_server
    const uint32_t IPC_PKT_FROM_PC_SERVER = 0x00030000;     // 数据包来源于pc_server
    const uint32_t IPC_PKT_FROM_PC_CLIENT = 0x00040000;     // 数据包来源于pc_client
    const uint32_t IPC_PKT_FROM_PHONE_CLIENT = 0x00050000;  // 数据包来源于phone_client

    // 音视频类型是实时视频还是历史视频，占9~12位。[0x00100000,0x00f00000]
    // const uint32_t IPC_PKT_MEDIA_MASK = 0x00f00000;
    // const uint32_t IPC_PKT_MEDIA_REAL = 0x00100000;    // 实时视频，默认
    // const uint32_t IPC_PKT_MEDIA_HISTORY = 0x00200000; // 历史视频

    // 其他自定义数据包，由各个模块自己定义使用，占1~8位。[0x01000000,0xff000000]。一共可以标识255个分类
    const uint32_t IPC_PKT_OTHER_MASK = 0xff000000;
    const uint32_t IPC_PKT_OTHER_THREAD_STOP = 0x01000000; // 子线程停止，目前用在RtmpThread::CreateTerminateTask中

} // namespace ipc

#endif // IPC_PACKET_TYPE_H