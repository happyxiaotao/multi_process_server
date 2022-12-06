#ifndef PUB_SUB_PUBLISHER_H
#define PUB_SUB_PUBLISHER_H

#include "Channel.h"
// 目前是单一发布者，拥有多个通道
namespace forward
{
    class Publisher;
    typedef std::shared_ptr<Publisher> PublisherPtr;
    class Publisher
    {
    public:
        Publisher() {}
        virtual ~Publisher() {}

    public:
        //发布消息
        void Publish(const channel_id_t &channel_id, const Message &message);
        inline bool Exists(const channel_id_t &channel_id) { return m_mapChannel.find(channel_id) != m_mapChannel.end(); }
        inline void AddChannel(const ChannelPtr &channel) { m_mapChannel[channel->GetChannelId()] = channel; }
        inline void DelChannel(const channel_id_t &channel_id) { m_mapChannel.erase(channel_id); }

        // 直接创建
        void CreateChannel(const channel_id_t &channel_id);

        // 添加订阅者
        void AddSubscriber(const channel_id_t &channel_id, const SubscriberPtr &subscriber);

        //删除指定通话的会话
        void DelSubscriber(const channel_id_t &channel_id, const SubscriberPtr &subscriber);
        void DelSubscriber(const channel_id_t &channel_id, const session_id_t &session_id);
        //删除所有通道中的此会话
        void DelSubscriber(const SubscriberPtr subscriber);
        void DelSubscriber(const session_id_t &session_id);

    private:
        const ChannelPtr &GetChannelPtr(const channel_id_t &channel_id) const;

    protected:
        std::map<channel_id_t, ChannelPtr> m_mapChannel; //通道列表
    };
} // namespace forward

#endif // PUB_SUB_PUBLISHER_H