#ifndef TCP_ERROR_H
#define TCP_ERROR_H
#include <stdint.h>
enum TcpErrorType : uint32_t
{
    TCP_ERROR_DISCONNECT = 1,
    TCP_ERROR_NET_ERROR = 2, // 网络错误，read返回-1的情况

    TCP_ERROR_USER_BUFFER_FULL, // 用户自定义缓存已满

    TCP_ERROR_INVALID_PACKET, //非法的数据包
};
#endif // TCP_ERROR_H