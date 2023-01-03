#include "RtmpMgr.h"
// #include "../../../core/log/Log.hpp"
#include "RtmpMgr.h"
#include "../../../jt1078/Jt1078Util.h"

RtmpMgr::RtmpMgr()
{
}

RtmpMgr::~RtmpMgr()
{
}

std::shared_ptr<RtmpClient> RtmpMgr::GetRtmpClient(device_id_t device_id)
{
    std::shared_ptr<RtmpClient> client = nullptr;
    auto iter = m_mapClient.find(device_id);
    if (iter != m_mapClient.end())
    {
        return iter->second;
    }
    return client;
}

void RtmpMgr::CreateRtmpClient(device_id_t device_id, const std::string &rtmp_url_prefix)
{
    auto iter = m_mapClient.find(device_id);
    if (iter != m_mapClient.end())
    {
        return;
    }
    m_mapClient[device_id] = std::make_shared<RtmpClient>(device_id, rtmp_url_prefix);
}

void RtmpMgr::CreateRtmpClient(const char *pDeviceId, size_t len, const std::string &rtmp_url_prefix)
{
    (void)len;
    CreateRtmpClient(GenerateDeviceIdByBuffer(pDeviceId), rtmp_url_prefix);
}

void RtmpMgr::ReleaseRtmpClient(device_id_t device_id)
{
    m_mapClient.erase(device_id);
}

void RtmpMgr::ReleaseRtmpClient(const char *pDeviceId, size_t len)
{
    (void)len;
    ReleaseRtmpClient(GenerateDeviceIdByBuffer(pDeviceId));
}
