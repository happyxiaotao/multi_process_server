#ifndef PUB_SUB_CHANNEL_H
#define PUB_SUB_CHANNEL_H

#include <string>
#include <memory>
#include <map>
#include "Subscriber.h"
namespace pub_sub
{
    typedef std::string channel_name_t;

    class Channel;
    typedef std::shared_ptr<Channel> ChannelPtr;

    class Channel
    {
    public:
        Channel(const channel_name_t &name) : m_channel_name(name) {}
        ~Channel() {}

    public:
        channel_name_t GetChannelName() { return m_channel_name; }

        void AddSubscriber(const SubscriberPtr &subscriber) { m_mapSubscriber[subscriber->GetSubscriberName()] = subscriber; }
        void DelSubscriber(const subscriber_name_t &subscriber_name) { m_mapSubscriber.erase(subscriber_name); }

        void SendMsg(const Message &message)
        {
            for (auto &&subscriber : m_mapSubscriber)
            {
                subscriber.second->SendMsg(message);
            }
        }

    private:
        channel_name_t m_channel_name;
        std::map<subscriber_name_t, SubscriberPtr> m_mapSubscriber; //保存订阅者表
    };
} // namespace pub_sub

#endif // PUB_SUB_CHANNEL_H