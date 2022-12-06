
#include <algorithm>
#include "Publisher.h"
#include "../../core/log/Log.hpp"
namespace forward
{
    void Publisher::Publish(const channel_id_t &channel_id, const Message &message)
    {
        // Trace("Publisher::Publish ,channel_id:{:x}", channel_id);
        auto iter = m_mapChannel.find(channel_id);
        if (iter == m_mapChannel.end())
        {
            return;
        }
        auto &channel = iter->second;
        channel->SendMsg(message);
    }

    void Publisher::AddSubscriber(const channel_id_t &channel_id, const SubscriberPtr &subscriber)
    {
        auto &channel = GetChannelPtr(channel_id);
        if (channel == nullptr)
        {
            Warn("Publisher::AddSubscriber, not found channel,channel_id:{:x}", channel_id);
            return;
        }
        channel->AddSubscriber(subscriber);
    }

    void Publisher::DelSubscriber(const channel_id_t &channel_id, const SubscriberPtr &subscriber)
    {
        DelSubscriber(channel_id, subscriber->GetSessionId());
    }
    void Publisher::DelSubscriber(const channel_id_t &channel_id, const session_id_t &session_id)
    {
        auto &channel = GetChannelPtr(channel_id);
        if (channel == nullptr)
        {
            Warn("Publisher::DelSubscriber, not found channel,channel_id:{:x}", channel_id);
            return;
        }

        channel->DelSubscriber(session_id);
    }
    void Publisher::DelSubscriber(const SubscriberPtr subscriber)
    {
        DelSubscriber(subscriber->GetSessionId());
    }
    void Publisher::DelSubscriber(const session_id_t &session_id)
    {
        for (auto &&channel_iter : m_mapChannel)
        {
            channel_iter.second->DelSubscriber(session_id);
        }
    }

    const ChannelPtr &Publisher::GetChannelPtr(const channel_id_t &channel_id) const
    {
        static const ChannelPtr s_null_channel;
        auto iter = m_mapChannel.find(channel_id);
        if (iter == m_mapChannel.end())
        {
            return s_null_channel;
        }
        return iter->second;
    }
    void Publisher::CreateChannel(const channel_id_t &channel_id)
    {
        auto channel = std::make_shared<Channel>(channel_id);
        m_mapChannel[channel_id] = channel;
    }
} // namespace forward
