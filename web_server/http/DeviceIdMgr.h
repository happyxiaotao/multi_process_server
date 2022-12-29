#ifndef WEB_SERVER_DEVICE_ID_MGR_H
#define WEB_SERVER_DEVICE_ID_MGR_H

#include <set>
#include "../../jt1078/AV_Common_Define.h"

class DeviceIdMgr
{
public:
    DeviceIdMgr() {}
    ~DeviceIdMgr() {}

public:
    static device_id_t GetDeviceIdFromStr(const std::string &strDeviceId);

    bool Exists(const std::string &strDeviceId);
    bool Exists(device_id_t device_id) { return m_setDeviceId.find(device_id) != m_setDeviceId.end(); }

    void Insert(const std::string &strDeviceId);
    inline void Insert(device_id_t device_id) { m_setDeviceId.insert(device_id); }
    void Remove(const std::string &strDeviceId);
    inline void Remove(device_id_t device_id) { m_setDeviceId.erase(device_id); }

private:
    std::set<device_id_t> m_setDeviceId;
};
#endif // WEB_SERVER_DEVICE_ID_MGR_H