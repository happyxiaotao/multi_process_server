#include "PcPublisher.h"
#include "../core/log/Log.hpp"

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
        if (!pc->IsCanWrite())
        {
            Warn("PcPublisher::Publish, delete pc session because cannot write, session_id:{},device_id:{}", pc->GetSessionId(), pc->GetDeviceId());
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

size_t PcPublisher::SizeSubscriber(const device_id_t &device_id)
{
    auto iter = m_channels.find(device_id);
    if (iter == m_channels.end())
    {
        return 0;
    }
    auto &channel = iter->second;
    return channel->size();
}

std::set<device_id_t> PcPublisher::GetAllDeviceId(bool bFilterEmptySubscriber)
{
    std::set<device_id_t> result;
    for (auto &iter : m_channels)
    {
        if (bFilterEmptySubscriber) // 过滤掉空订阅者通道
        {
            if (!iter.second->empty())
            {
                result.insert(iter.first);
            }
        }
        else
        {
            result.insert(iter.first);
        }
    }
    return result;
}
