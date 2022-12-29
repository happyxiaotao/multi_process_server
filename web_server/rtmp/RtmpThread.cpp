#include "RtmpThread.h"
#include "../../core/log/Log.hpp"

RtmpThread::RtmpThread() : m_tmp_packet(nullptr)
{
    Trace("RtmpThread::RtmpThread");

    m_tmp_packet = new jt1078::packet_t();
    m_tmp_packet->m_header = nullptr; // 这里只是临时使用，不会实际开辟空间，保存数据
    m_tmp_packet->m_body = nullptr;   // 这里只是临时使用，m_body不会实际开辟空间，保存数据
}

RtmpThread::~RtmpThread()
{
    Trace("RtmpThread::~RtmpThread");
    m_tmp_packet->m_header = nullptr; // m_header和m_body需要赋值为nullptr，避免packet_t的析构函数，进行资源释放，导致程序崩溃
    m_tmp_packet->m_body = nullptr;
    delete m_tmp_packet;
}

void RtmpThread::PostPacket(const ipc::packet_t &packet)
{
    // Trace("RtmpThread::PostPacket, packet.seq_id={}, queue.size={}", packet.m_uPktSeqId, QueueSize());
    auto task_ptr = std::make_shared<ipc::packet_move_t>(packet);
    PostTask(std::move(task_ptr));
}

void RtmpThread::PostPacket(ipc::packet_move_t &&packet)
{
    PostTask(std::move(packet));
}

// 处理消息。
// 如果是JT1078数据包，则创建对应的RTMP流
// 如果是断开RTMP流的数据包，则释放掉对应的RTMP流
void RtmpThread::HandlerTask(const std::shared_ptr<ipc::packet_move_t> &task)
{
    // Trace("RtmpThread::HandlerTask, packet.seq_id={}", task->m_uPktSeqId);

    switch (task->m_uPktType)
    {
    // 订阅通道，只有订阅了通道，才会处理下一步的IPC_PKT_JT1078_PACKET数据包
    case ipc::IPC_PKT_SUBSCRIBE_DEVICD_ID:
    {
        m_rtmp_mgr.CreateRtmpClient(task->m_data, task->m_uDataLength);
        break;
    }
    // 停止订阅。
    case ipc::IPC_PKT_UNSUBSCRIBE_DEVICD_ID:
    {
        m_rtmp_mgr.ReleaseRtmpClient(task->m_data, task->m_uDataLength);
        break;
    }
    // 处理jt1078音视频数据
    case ipc::IPC_PKT_JT1078_PACKET:
    {
        const char *p = task->m_data;
        // 获取
        device_id_t *_device_id = (device_id_t *)p;
        p += sizeof(device_id_t);
        m_tmp_packet->m_header = (jt1078::header_t *)p;
        p += sizeof(jt1078::header_t);
        m_tmp_packet->m_body = const_cast<char *>(p); // 指向剩余空间

        const device_id_t &device_id = *_device_id;
        const jt1078::packet_t &packet = *m_tmp_packet;

        auto client = m_rtmp_mgr.GetRtmpClient(device_id);
        if (client != nullptr)
        {
            int ret = client->ProcessJt1078Packet(packet);
            if (ret < 0)
            {
                m_rtmp_mgr.ReleaseRtmpClient(device_id);
            }
        }
        break;
    }

    default:
        break;
    }
}

std::shared_ptr<ipc::packet_move_t> RtmpThread::CreateTerminateTask()
{
    return std::shared_ptr<ipc::packet_move_t>();
}

bool RtmpThread::IsTerminateTask(const std::shared_ptr<ipc::packet_move_t> &task)
{
    return false;
}