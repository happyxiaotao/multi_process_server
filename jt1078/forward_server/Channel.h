#ifndef FORWARD_SERVER_CHANNEL_H
#define FORWARD_SERVER_CHANNEL_H

#include <map>
#include <string>
#include "Subscriber.h"
#include "../AV_Common_Define.h"
#include "Message.h"
namespace forward
{
    class Channel;
    typedef std::shared_ptr<Channel> ChannelPtr;

    class Channel
    {
    public:
        Channel(channel_id_t channel_id) : m_channel_id(channel_id) {}
        ~Channel() {}

    public:
        inline size_t Size() { return m_mapSubscriber.size(); }
        inline bool Empty() { return m_mapSubscriber.empty(); }
        inline channel_id_t GetChannelId() { return m_channel_id; }
        inline void AddSubscriber(const SubscriberPtr &subscriber) { m_mapSubscriber[subscriber->GetSessionId()] = subscriber; }
        inline void DelSubscriber(session_id_t session_id) { m_mapSubscriber.erase(session_id); }

        void SendMsg(const Message &message);

    private:
        channel_id_t m_channel_id;
        std::map<session_id_t, SubscriberPtr> m_mapSubscriber;
    };
} // namespace forward
#endif // FORWARD_SERVER_CHANNEL_H