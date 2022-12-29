#ifndef RTMP_THREAD_H
#define RTMP_THREAD_H

#include "../thread/WorkThread.hpp"
#include "../../core/ipc_packet/IpcPacket.h"
#include "RtmpStream/RtmpMgr.h"
#include "../../jt1078/jt1078_server/Jt1078Packet.h"
class RtmpThread : public WorkThread<ipc::packet_move_t>
{
public:
    RtmpThread();
    virtual ~RtmpThread() override;

public:
    void PostPacket(const ipc::packet_t &packet);
    void PostPacket(ipc::packet_move_t &&packet);

protected:
    // 派生类实现：处理任务
    virtual void HandlerTask(const std::shared_ptr<ipc::packet_move_t> &task);
    // 派生类实现：创建表示退出线程的task
    virtual std::shared_ptr<ipc::packet_move_t> CreateTerminateTask();
    // 派生类实现：判断是否应该退出线程
    virtual bool IsTerminateTask(const std::shared_ptr<ipc::packet_move_t> &task);

private:
    RtmpMgr m_rtmp_mgr;
    jt1078::packet_t *m_tmp_packet; // 临时的packet_t
};

#endif // RTMP_THREAD_H