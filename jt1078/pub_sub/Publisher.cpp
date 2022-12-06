
#include <algorithm>
#include "Publisher.h"
#include "../../core/log/Log.hpp"
namespace pub_sub
{
    void Publisher::Publish(const channel_name_t &channel_name, const Message &message)
    {
        Trace("Publisher::Publish ,channel_name:{}", channel_name);
        auto iter = m_mapChannel.find(channel_name);
        if (iter == m_mapChannel.end())
        {
            return;
        }
        auto &channel = iter->second;
        channel->SendMsg(message);
    }

    void Publisher::DelSubscriber(const channel_name_t &channel_name, const subscriber_name_t &subscriber_name)
    {
        auto channel = GetChannelPtr(channel_name);
        if (channel == nullptr)
        {
            Trace("not found channel,name:{}", channel_name);
            return;
        }

        channel->DelSubscriber(subscriber_name);
    }
    const ChannelPtr Publisher::GetChannelPtr(const channel_name_t &channel_name) const
    {
        auto iter = m_mapChannel.find(channel_name);
        if (iter == m_mapChannel.end())
        {
            return nullptr;
        }
        return iter->second;
    }
} // namespace pub_sub
