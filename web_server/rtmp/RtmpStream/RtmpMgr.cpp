#include "RtmpMgr.h"
#include "../../../core/log/Log.hpp"
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

void RtmpMgr::CreateRtmpClient(device_id_t device_id)
{
    auto iter = m_mapClient.find(device_id);
    if (iter != m_mapClient.end())
    {
        return;
    }
    m_mapClient[device_id] = std::make_shared<RtmpClient>(device_id);
    Trace("RtmpMgr::CreateRtmpClient, Create RtmpClient,device_id:{:014x}, map.size()={}", device_id, Size());
}

void RtmpMgr::CreateRtmpClient(const char *pDeviceId, size_t len)
{
    (void)len;
    CreateRtmpClient(GenerateDeviceIdByBuffer(pDeviceId));
}

void RtmpMgr::ReleaseRtmpClient(device_id_t device_id)
{
    m_mapClient.erase(device_id);
    Trace("RtmpMgr::ReleaseRtmpClient, Release RtmpClient,device_id:{:014x}, map.size()={}", device_id, Size());
}

void RtmpMgr::ReleaseRtmpClient(const char *pDeviceId, size_t len)
{
    (void)len;
    ReleaseRtmpClient(GenerateDeviceIdByBuffer(pDeviceId));
}
