#ifndef RTMP_MGR_H
#define RTMP_MGR_H

#include <map>
#include <memory>
#include "RtmpClient.h"
#include "../../../jt1078/AV_Common_Define.h"

class RtmpMgr
{
public:
    RtmpMgr();
    ~RtmpMgr();

public:
    size_t Size() { return m_mapClient.size(); }

    // 根据对应device_id，获取RTMP流，如果不存在，返回nullptr
    std::shared_ptr<RtmpClient> GetRtmpClient(device_id_t device_id);

    // 创建对应的RTMP流
    void CreateRtmpClient(device_id_t device_id, const std::string &rtmp_url_prefix);
    void CreateRtmpClient(const char *pDeviceId, size_t len, const std::string &rtmp_url_prefix);
    void CreateRtmpClient(const std::string &strDeviceId, const std::string &rtmp_url_prefix) { return CreateRtmpClient(strDeviceId.c_str(), strDeviceId.size(), rtmp_url_prefix); }

    // 删除对应RTMP流
    void ReleaseRtmpClient(device_id_t device_id);
    void ReleaseRtmpClient(const char *pDeviceId, size_t len);
    void ReleaseRtmpClient(const std::string &strDeviceId) { return ReleaseRtmpClient(strDeviceId.c_str(), strDeviceId.size()); }

private:
    std::map<device_id_t, std::shared_ptr<RtmpClient>> m_mapClient;
};

#endif // RTMP_MGR_H