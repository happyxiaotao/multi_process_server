#ifndef PC_SERVER_PC_PUBLISHER_H
#define PC_SERVER_PC_PUBLISHER_H

#include <map>
#include <set>
#include "PcSession.h"
#include "../jt1078/AV_Common_Define.h"
class PcPublisher
{
    typedef std::map<session_id_t, PcSessionPtr> Channel;
    typedef std::shared_ptr<Channel> ChannelPtr;

public:
    PcPublisher();
    ~PcPublisher();

public:
    void Publish(const device_id_t &device_id, const ipc::packet_t &packet);
    void AddSubscriber(const device_id_t &device_id, const PcSessionPtr &pc);

    // 删除指定通话的会话
    void DelSubscriber(const device_id_t &device_id, const PcSessionPtr &pc);
    // 删除所有通道中的此会话
    void DelSubscriber(const PcSessionPtr &pc);

    // 返回某个device_id的订阅者个数
    size_t SizeSubscriber(const device_id_t &device_id);
    bool IsEmptySubscriber(const device_id_t &device_id) { return SizeSubscriber(device_id) == 0; }

    // 返回所有订阅的device_id
    std::set<device_id_t> GetAllDeviceId();

private:
    std::map<device_id_t, ChannelPtr> m_channels;
};
#endif // PC_SERVER_PC_PUBLISHER_H