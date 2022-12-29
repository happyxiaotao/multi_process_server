#include "DeviceIdMgr.h"
#include "../../jt1078/Jt1078Util.h"

device_id_t DeviceIdMgr::GetDeviceIdFromStr(const std::string &strDeviceId)
{
    return GenerateDeviceId(strDeviceId);
}

bool DeviceIdMgr::Exists(const std::string &strDeviceId)
{
    return Exists(GenerateDeviceId(strDeviceId));
}

void DeviceIdMgr::Insert(const std::string &strDeviceId)
{
    Insert(GenerateDeviceId(strDeviceId));
}

void DeviceIdMgr::Remove(const std::string &strDeviceId)
{
    Remove(GenerateDeviceId(strDeviceId));
}