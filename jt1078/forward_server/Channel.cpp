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

            if (!subscriber->IsCanWrite())
            {
                Error("Channel::SendMsg failed, check cannot write, release subscriber,channel_id:{:x},session_id:{}", GetChannelId(), iter->second->GetSessionId());
                iter = m_mapSubscriber.erase(iter);
            }
            else
            {
                //优先发送上次未发送成功的数据包
                if (subscriber->HasPendingMsg())
                {
                    Trace("pending_list size:{},session_id:{}", subscriber->PendingMsgSize(), subscriber->GetSessionId());
                    if (!subscriber->SendPendingMsg() && !subscriber->IsCanWrite())
                    {
                        Error("Channel::SendMsg failed, cannot write pending, release subscriber,channel_id:{:x},session_id:{}", GetChannelId(), iter->second->GetSessionId());
                        iter = m_mapSubscriber.erase(iter);
                        continue;
                    }
                }
                if (subscriber->HasPendingMsg())
                {
                    subscriber->AddPendingMsg(message);
                }
                else
                {
                    if ((subscriber->SendMsg(message) < 0) && !subscriber->IsCanWrite())
                    {
                        Error("Channel::SendMsg failed, cannot write msg, release subscriber,channel_id:{:x},session_id:{}", GetChannelId(), iter->second->GetSessionId());
                        iter = m_mapSubscriber.erase(iter);
                        continue;
                    }
                }
                ++iter;
            }
        }
    }
} // namespace forward
