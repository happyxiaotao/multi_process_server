#include "Subscriber.h"
#include "../../core/log/Log.hpp"

namespace pub_sub
{
    void Subscriber::SendMsg(const Message &message)
    {
        Trace("Subscriber name:{}, message:{}", m_subscriber_name, message.GetMsg());
    }
} // namespace pub_sub
