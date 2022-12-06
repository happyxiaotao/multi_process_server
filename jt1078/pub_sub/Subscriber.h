#ifndef PUB_SUB_SUBSCRIBER_H
#define PUB_SUB_SUBSCRIBER_H
#include <memory>
#include "Message.h"
namespace pub_sub
{
    typedef std::string subscriber_name_t;
    class Subscriber;
    typedef std::shared_ptr<Subscriber> SubscriberPtr;
    class Subscriber
    {
    public:
        Subscriber(const subscriber_name_t &name) : m_subscriber_name(name) {}

    public:
        subscriber_name_t GetSubscriberName() { return m_subscriber_name; }

        void SendMsg(const Message &message);

    private:
        subscriber_name_t m_subscriber_name;
    };
} // namespace pub_sub

#endif // PUB_SUB_SUBSCRIBER_H