#include "PcPublisher.h"

PcPublisher::PcPublisher()
{
}
PcPublisher::~PcPublisher()
{
}

void PcPublisher::Publish(const device_id_t &device_id, const ipc::packet_t &packet)
{
    auto iter = m_channels.find(device_id);
    if (iter == m_channels.end())
    {
        return;
    }
    auto &channel = iter->second;
    for (auto iter = channel->begin(); iter != channel->end();)
    {
        auto &pc = iter->second;
        if (pc->IsCanWrite())
        {
            pc->SendPacket(packet);
        }
        // 释放不能写入数据的，连接
        if (pc->IsCanWrite())
        {
            iter = channel->erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}
void PcPublisher::AddSubscriber(const device_id_t &device_id, const PcSessionPtr &pc)
{
    ChannelPtr channel = nullptr;
    auto iter = m_channels.find(device_id);
    if (iter != m_channels.end())
    {
        channel = iter->second;
    }

    if (channel == nullptr)
    {
        channel = std::make_shared<Channel>();
        m_channels[device_id] = channel;
    }
    channel->insert(std::pair<session_id_t, PcSessionPtr>(pc->GetSessionId(), pc));
}
void PcPublisher::DelSubscriber(const device_id_t &device_id, const PcSessionPtr &pc)
{
    auto iter = m_channels.find(device_id);
    if (iter == m_channels.end())
    {
        return;
    }
    ChannelPtr &channel = iter->second;
    channel->erase(pc->GetSessionId());
}
void PcPublisher::DelSubscriber(const PcSessionPtr &pc)
{
    for (auto &&iter : m_channels)
    {
        auto &channel = iter.second;
        channel->erase(pc->GetSessionId());
    }
}