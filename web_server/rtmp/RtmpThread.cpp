#include "RtmpThread.h"
#include "../../core/log/Log.hpp"
#include "../../core/ini_config.h"

RtmpThread::RtmpThread(bool need_push_realtime_data)
    : m_tmp_packet(nullptr), m_need_push_realtime_data(need_push_realtime_data)
{
    m_tmp_packet = new jt1078::packet_t();
    m_tmp_packet->m_header = nullptr; // 这里只是临时使用，不会实际开辟空间，保存数据
    m_tmp_packet->m_body = nullptr;   // 这里只是临时使用，m_body不会实际开辟空间，保存数据

    if (m_need_push_realtime_data) // 设置实时视频推送url
    {
        m_rtmp_url_prefix = g_ini->Get("rtmp", "realtime_url_prefix", "");
    }
    else // 设置历史视频推送url
    {
        m_rtmp_url_prefix = g_ini->Get("rtmp", "history_url_prefix", "");
    }
}

RtmpThread::~RtmpThread()
{
    Trace("RtmpThread::~RtmpThread, need_push_realtime_data:{}", m_need_push_realtime_data);
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

    uint32_t uIpcPktType = task->m_uPktType & ipc::IPC_PKT_TYPE_MASK;

    switch (uIpcPktType)
    {
    // 订阅通道，只有订阅了通道，才会处理下一步的IPC_PKT_TYPE_JT1078_PACKET数据包
    case ipc::IPC_PKT_TYPE_SUBSCRIBE_DEVICE_ID:
    {
        // m_rtmp_mgr.CreateRtmpClient(task->m_data, task->m_uDataLength, m_rtmp_url_prefix);
        std::string strDeviceId(task->m_data, task->m_uDataLength);
        m_rtmp_mgr.CreateRtmpClient(strDeviceId, m_rtmp_url_prefix);
        Trace("RtmpThread::HandlerTask,CreateRtmpClient, is_realtime_server:{},m_rtmp_url_prefix:{},device_id:{}", m_need_push_realtime_data, m_rtmp_url_prefix, strDeviceId);
        break;
    }
    // 停止订阅。
    case ipc::IPC_PKT_TYPE_UNSUBSCRIBE_DEVICE_ID:
    {
        // m_rtmp_mgr.ReleaseRtmpClient(task->m_data, task->m_uDataLength);
        std::string strDeviceId(task->m_data, task->m_uDataLength);
        m_rtmp_mgr.ReleaseRtmpClient(strDeviceId);
        Trace("RtmpThread::HandlerTask,ReleaseRtmpClient, is_realtime_server:{},strDeviceId:{}", m_need_push_realtime_data, strDeviceId);
        break;
    }
    // 处理jt1078音视频数据
    case ipc::IPC_PKT_TYPE_JT1078_PACKET:
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
                Trace("RtmpThread::HandlerTask,ReleaseRtmpClient,is_realtime_server:{},ProcessJt1078Packet failed,strDeviceId:{:x014}", m_need_push_realtime_data, device_id);
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
    auto move_packet = std::make_shared<ipc::packet_move_t>();
    move_packet->m_uPktType = ipc::IPC_PKT_OTHER_THREAD_STOP; // 设置标识为停止线程
    return move_packet;
}

bool RtmpThread::IsTerminateTask(const std::shared_ptr<ipc::packet_move_t> &task)
{
    return (task->m_uPktType & ipc::IPC_PKT_OTHER_MASK) == ipc::IPC_PKT_OTHER_THREAD_STOP;
}