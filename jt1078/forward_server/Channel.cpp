#include "Channel.h"
#include <set>
#include "../../core/log/Log.hpp"

namespace forward
{
    void Channel::SendMsg(const Message &message)
    {
        for (auto iter = m_mapSubscriber.begin(); iter != m_mapSubscriber.end();)
        {
            auto &subscriber = iter->second;
            if (subscriber->IsCanWrite())
            {
                subscriber->SendMsg(message);
            }

            if (!subscriber->IsCanWrite())
            {
                Error("Channel::SendMsg failed, check cannot write, release subscriber,channel_id:{:x},session_id:{}", GetChannelId(), iter->second->GetSessionId());
                iter = m_mapSubscriber.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
} // namespace forward
