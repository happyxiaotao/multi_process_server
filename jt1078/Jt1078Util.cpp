#include "Jt1078Util.h"

std::string GenerateDeviceIdStr(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber)
{
    if (pSimCardNumber == nullptr)
    {
        return "";
    }
    char szDeviceId[15]; // 终端id最多占用14位（7*2），另外添加一个'\0'
    sprintf(szDeviceId, "%02x%02x%02x%02x%02x%02x%02x",
            pSimCardNumber[0], pSimCardNumber[1], pSimCardNumber[2], pSimCardNumber[3], pSimCardNumber[4], pSimCardNumber[5],
            uLogicChannelNumber);
    szDeviceId[14] = '\0';
    return std::string(szDeviceId, 14);
}
device_id_t GenerateDeviceId(const std::string &strDeviceId)
{
    return strtoull(strDeviceId.c_str(), nullptr, 16); // 返回16进制的device_id
}

device_id_t GenerateDeviceId(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber)
{
    return GenerateDeviceId(GenerateDeviceIdStr(pSimCardNumber, uLogicChannelNumber));
}
device_id_t GenerateDeviceIdByBuffer(const char *buffer)
{
    return strtoull(buffer, nullptr, 16);
}

std::string GenerateDeviceIdStrByDeviceId(device_id_t device_id)
{
    char buf[16];
    int nret = snprintf(buf, sizeof(buf) - 1, "%014lx", device_id);
    buf[nret] = '\0';
    return std::string(buf, nret);
}

bool CheckDeviceIdStrFmt(const std::string &strDeviceId)
{
    if (strDeviceId.size() != 14)
    {
        return false;
    }
    for (int i = 0; i < 14; i++)
    {
        if (!isdigit(strDeviceId[i]))
        {
            return false;
        }
    }
    return true;
}
