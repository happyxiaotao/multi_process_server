#ifndef IPC_PACKET_TYPE_H
#define IPC_PACKET_TYPE_H
#include <stdint.h>
namespace ipc
{
    enum IpcPktType : uint32_t
    {
        IPC_PKT_INVALID_PACKET = 0,

        IPC_PKT_HEARTBEAT, //心跳包

        IPC_PKT_SUBSCRIBE_ICCID,   //订阅iccid
        IPC_PKT_UNSUBSCRIBE_ICCID, //取消订阅iccid

        IPC_PKT_JT1078_PACKET, // jt1078数据包

    };
} // namespace ipc

#endif // IPC_PACKET_TYPE_H