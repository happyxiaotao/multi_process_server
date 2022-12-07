#ifndef JT1078_SERVER_UTIL_H
#define JT1078_SERVER_UTIL_H

#include "../AV_Common_Define.h"
//   根据sim和logic生成唯一device_id(占7个字节)
std::string GenerateDeviceIdStr(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber);
device_id_t GenerateDeviceId(const std::string &strDeviceId);
device_id_t GenerateDeviceId(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber);
device_id_t GenerateDeviceIdByBuffer(const char *buffer);
std::string GenerateDeviceIdStrByDeviceId(device_id_t device_id);

#endif // JT1078_SERVER_UTIL_H