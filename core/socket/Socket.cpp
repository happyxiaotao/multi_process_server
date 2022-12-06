#include <event2/event.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include "./Socket.h"
namespace sock
{

    evutil_socket_t GetTcpListenSocket()
    {
        evutil_socket_t fd = GetTcpClientSocket();
        if (fd == INVALID_SOCKET)
        {
            return INVALID_SOCKET;
        }
        evutil_make_listen_socket_reuseable(fd);
        evutil_make_listen_socket_reuseable_port(fd);
        return fd;
    }
    evutil_socket_t GetTcpClientSocket()
    {
        evutil_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            return INVALID_SOCKET;
        }
        evutil_make_socket_nonblocking(fd);
        evutil_make_socket_closeonexec(fd);
        return fd;
    }

    bool SetSocketIpv4Addr(const std::string &ip, u_short port, struct sockaddr_in &addr)
    {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        return evutil_inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) == 1;
    }
    bool GetIpPortFromSockaddr(struct sockaddr_in &addr, std::string &ip, u_short &port)
    {
        ip.clear();
        port = 0;
        char szIp[128];
        memset(szIp, 0, sizeof(szIp));
        const char *cp = evutil_inet_ntop(AF_INET, &addr.sin_addr, szIp, sizeof(szIp));
        if (cp != nullptr)
        {
            ip.assign(szIp);
            port = ntohs(addr.sin_port);
            return true;
        }
        return false;
    }
    int SocketConnect(evutil_socket_t *fd_ptr, const struct sockaddr *sa, int socklen)
    {

#if EAGAIN == EWOULDBLOCK
#define EVUTIL_ERR_IS_EAGAIN(e) \
    ((e) == EAGAIN)
#else
#define EVUTIL_ERR_IS_EAGAIN(e) \
    ((e) == EAGAIN || (e) == EWOULDBLOCK)
#endif

/* True iff e is an error that means a read/write operation can be retried. */
#define EVUTIL_ERR_RW_RETRIABLE(e) \
    ((e) == EINTR || EVUTIL_ERR_IS_EAGAIN(e))
/* True iff e is an error that means an connect can be retried. */
#define EVUTIL_ERR_CONNECT_RETRIABLE(e) \
    ((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define EVUTIL_ERR_ACCEPT_RETRIABLE(e) \
    ((e) == EINTR || EVUTIL_ERR_IS_EAGAIN(e) || (e) == ECONNABORTED)

/* True iff e is an error that means the connection was refused */
#define EVUTIL_ERR_CONNECT_REFUSED(e) \
    ((e) == ECONNREFUSED)

        int made_fd = 0;

        if (*fd_ptr < 0)
        {
            if ((*fd_ptr = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
                goto err;
            made_fd = 1;
            if (evutil_make_socket_nonblocking(*fd_ptr) < 0)
            {
                goto err;
            }
        }

        if (connect(*fd_ptr, sa, socklen) < 0)
        {
            int e = evutil_socket_geterror(*fd_ptr);
            if (EVUTIL_ERR_CONNECT_RETRIABLE(e))
                return 0;
            if (EVUTIL_ERR_CONNECT_REFUSED(e))
                return 2;
            goto err;
        }
        else
        {
            return 1;
        }

    err:
        if (made_fd)
        {
            evutil_closesocket(*fd_ptr);
            *fd_ptr = -1;
        }
        return -1;
    }
} // namespace socket
