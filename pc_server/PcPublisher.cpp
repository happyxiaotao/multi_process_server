#include "PcPublisher.h"

PcPublisher::PcPublisher()
{
}
PcPublisher::~PcPublisher()
{
}

void PcPublisher::Publish(const device_id_t&device_id, const ipc::packet_t &packet)
{
    auto iter = m_channels.find(device_id);
    if (iter == m_channels.end())
    {
        return;
    }
    auto &channel = iter->second;
    for (auto &&iter : *channel)
    {
        auto &pc = iter.second;
        pc->SendPacket(packet);
    }
}
void PcPublisher::AddSubscriber(const device_id_t&device_id, const PcSessionPtr &pc)
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
void PcPublisher::DelSubscriber(const device_id_t&device_id, const PcSessionPtr &pc)
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