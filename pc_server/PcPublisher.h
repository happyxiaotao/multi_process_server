#ifndef PC_SERVER_PC_PUBLISHER_H
#define PC_SERVER_PC_PUBLISHER_H

#include <map>
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
    void Publish(const iccid_t &iccid, const ipc::packet_t &packet);
    void AddSubscriber(const iccid_t &iccid, const PcSessionPtr &pc);

    //删除指定通话的会话
    void DelSubscriber(const iccid_t &iccid, const PcSessionPtr &pc);
    //删除所有通道中的此会话
    void DelSubscriber(const PcSessionPtr &pc);

private:
    std::map<iccid_t, ChannelPtr> m_channels;
};
#endif // PC_SERVER_PC_PUBLISHER_H