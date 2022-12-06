#ifndef PUB_SUB_PUBLISHER_H
#define PUB_SUB_PUBLISHER_H

#include "Channel.h"
// 目前是单一发布者，拥有多个通道
namespace pub_sub
{
    class Publisher;
    typedef std::shared_ptr<Publisher> PublisherPtr;
    class Publisher
    {
    public:
        Publisher(const std::string &name) : m_publisher_name(name) {}

    public:
        //发布消息
        void Publish(const channel_name_t &channel_name, const Message &message);
        inline void AddChannel(const ChannelPtr &channel) { m_mapChannel[channel->GetChannelName()] = channel; }
        inline void DelChannel(const channel_name_t &channel_name) { m_mapChannel.erase(channel_name); }

        void DelSubscriber(const channel_name_t &channel_name, const subscriber_name_t &subscriber_name);

    private:
        const ChannelPtr GetChannelPtr(const channel_name_t &channel_name) const;

    private:
        std::string m_publisher_name;
        std::map<channel_name_t, ChannelPtr> m_mapChannel; //通道列表
    };
} // namespace pub_sub

#endif // PUB_SUB_PUBLISHER_H