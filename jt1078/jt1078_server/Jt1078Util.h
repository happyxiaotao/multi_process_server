#ifndef JT1078_SERVER_UTIL_H
#define JT1078_SERVER_UTIL_H

#include "../AV_Common_Define.h"
//   根据sim和logic生成唯一iccid(占7个字节)
std::string GenerateIccidStr(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber);
iccid_t GenerateIccid(const std::string &strIccid);
iccid_t GenerateIccid(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber);
iccid_t GenerateIccidByBuffer(const char *buffer);
std::string GenerateIccidStrByIccid(iccid_t iccid);

#endif // JT1078_SERVER_UTIL_H