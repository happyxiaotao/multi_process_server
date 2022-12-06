#ifndef SWORD_SOCKET_H
#define SWORD_SOCKET_H
#include <string>
#include <event2/event.h>

constexpr evutil_socket_t INVALID_SOCKET = -1;
constexpr u_short INVALID_PORT = -1;

namespace sock
{
    evutil_socket_t GetTcpListenSocket();
    evutil_socket_t GetTcpClientSocket();
    bool SetSocketIpv4Addr(const std::string &ip, u_short port, struct sockaddr_in &addr);
    bool GetIpPortFromSockaddr(struct sockaddr_in &addr, std::string &ip, u_short &port);

    inline uint16_t networkToHost16(uint16_t value) { return ntohs(value); }
    inline uint32_t networkToHost32(uint32_t value) { return ntohl(value); }
    inline uint64_t networkToHost64(uint64_t value)
    {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        return (((uint64_t)ntohl(value)) << 32 | ntohl(value >> 32));
#else
        return value;
#endif
    }
    // 从libevent的evutil.c拷贝evutil_socket_connect_函数实现
    /* XXX we should use an enum here. */
    /* 2 for connection refused, 1 for connected, 0 for not yet, -1 for error. */
    int SocketConnect(evutil_socket_t *fd_ptr, const struct sockaddr *sa, int socklen);
} // namespace socket

#endif // SWORD_SOCKET_H