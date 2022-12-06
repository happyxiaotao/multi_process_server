#include "Jt1078Util.h"

std::string GenerateIccidStr(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber)
{
    if (pSimCardNumber == nullptr)
    {
        return "";
    }
    char szSessionName[15]; // 终端id最多占用14位（7*2），另外添加一个'\0'
    sprintf(szSessionName, "%02x%02x%02x%02x%02x%02x%02x",
            pSimCardNumber[0], pSimCardNumber[1], pSimCardNumber[2], pSimCardNumber[3], pSimCardNumber[4], pSimCardNumber[5],
            uLogicChannelNumber);
    szSessionName[14] = '\0';
    return std::string(szSessionName, 14);
}
iccid_t GenerateIccid(const std::string &strIccid)
{
    return strtoull(strIccid.c_str(), nullptr, 16); //返回16进制的iccid
}

iccid_t GenerateIccid(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber)
{
    return GenerateIccid(GenerateIccidStr(pSimCardNumber, uLogicChannelNumber));
}
iccid_t GenerateIccidByBuffer(const char *buffer)
{
    return strtoull(buffer, nullptr, 16);
}

std::string GenerateIccidStrByIccid(iccid_t iccid)
{
    char buf[16];
    int nret = snprintf(buf, sizeof(buf) - 1, "%014lx", iccid);
    buf[nret] = '\0';
    return std::string(buf, nret);
}