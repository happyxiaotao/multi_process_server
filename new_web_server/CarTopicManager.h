#ifndef CAR_TOPIC_MANAGER_H
#define CAR_TOPIC_MANAGER_H

#include <set>
#include <map>
#include <list>
#include "../jt1078/AV_Common_Define.h"
#include "CarWebSocketServer.h"

class CarTopic
{
    typedef websocketpp::connection_hdl hdl_t;
    typedef std::set<hdl_t, std::owner_less<hdl_t>> hdl_set;

public:
    CarTopic(const device_id_t device_id, CarWebSocketServer *websocket) : m_device_id(device_id), m_websocket(websocket)
    {
    }

public:
    void AddSubscriber(hdl_t hdl)
    {
        m_hdls.insert(hdl);
    }
    void RemoveSubscriber(hdl_t hdl)
    {
        m_hdls.erase(hdl);
    }
    void Publish(const char *data, size_t len)
    {
        if (m_websocket == nullptr)
        {
            return;
        }

        for (auto &hdl : m_hdls)
        {
            m_websocket->SendToHdl(hdl, data, len);
        }
    }
    size_t Count()
    {
        return m_hdls.size();
    }

    bool IsEmpty() { return Count() == 0; }

private:
    device_id_t m_device_id;
    CarWebSocketServer *m_websocket;
    hdl_set m_hdls;
};

class CarTopicManager
{
public:
    CarTopicManager(CarWebSocketServer *websocket) : m_websocket(websocket)
    {
    }

public:
    bool ExistsTopic(device_id_t device_id)
    {
        return m_topics.find(device_id) != m_topics.end();
    }
    std::shared_ptr<CarTopic> GetTopic(device_id_t device_id, bool bCreateIfNotExists = false)
    {
        std::shared_ptr<CarTopic> topic{nullptr};
        auto iter = m_topics.find(device_id);
        if (iter != m_topics.end())
        {
            topic = iter->second;
        }
        if (topic == nullptr && bCreateIfNotExists)
        {
            topic = std::make_shared<CarTopic>(device_id, m_websocket);
            m_topics[device_id] = topic;
        }
        return topic;
    }

    void RemoveTopic(device_id_t device_id)
    {
        auto iter = m_topics.find(device_id);
        if (iter != m_topics.end())
        {
            m_topics.erase(iter);
        }
    }

    void RemoveTopics(const std::list<device_id_t> &device_ids)
    {
        for (auto &device_id : device_ids)
        {
            m_topics.erase(device_id);
        }
    }

    // 订阅者离开，释放所有订阅的话题
    void RemoveSubscriber(websocketpp::connection_hdl hdl)
    {
        for (auto topic : m_topics)
        {
            topic.second->RemoveSubscriber(hdl);
        }
    }

    // 获取没有订阅者的话题device_id列表
    std::list<device_id_t> GetEmptyTopicList()
    {
        std::list<device_id_t> list;
        for (auto iter = m_topics.begin(); iter != m_topics.end(); ++iter)
        {
            if (iter->second->IsEmpty())
            {
                list.push_back(iter->first);
            }
        }
        return list;
    }

private:
    CarWebSocketServer *m_websocket;
    std::map<device_id_t, std::shared_ptr<CarTopic>> m_topics;
};

#endif // CAR_TOPIC_MANAGER_H