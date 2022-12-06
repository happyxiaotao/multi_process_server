#include "Publisher.h"
#include "../../ini_config.h"
#include "../../core/log/Log.hpp"

INIReader *g_ini;

int main()
{
    g_ini = new INIReader();

    spdlog::default_logger()->set_level(spdlog::level::trace);

    pub_sub::PublisherPtr publisher = std::make_shared<pub_sub::Publisher>("publisher");
    {
        pub_sub::ChannelPtr channel = std::make_shared<pub_sub::Channel>("channel");
        pub_sub::SubscriberPtr subscriber_first = std::make_shared<pub_sub::Subscriber>("first");
        pub_sub::SubscriberPtr subscriber_second = std::make_shared<pub_sub::Subscriber>("second");
        channel->AddSubscriber(subscriber_first);
        channel->AddSubscriber(subscriber_second);
        publisher->AddChannel(channel);
    }
    pub_sub::channel_name_t channel_name("channel");
    pub_sub::Message message("abcdefg");
    publisher->Publish(channel_name, message);

    publisher->DelSubscriber(channel_name, "first");
    publisher->Publish(channel_name, message);

    publisher->DelChannel(channel_name);
    publisher->Publish(channel_name, message);
}