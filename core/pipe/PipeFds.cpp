#include <unistd.h>
#include <fcntl.h> /* Obtain O_* constant definitions */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "PipeFds.h"
#include "../core/log/Log.hpp"

#define INVALID_SOCKET_FD (-1)
PipeFds::PipeFds()
{
    m_fds_master[0] = INVALID_SOCKET_FD;
    m_fds_master[1] = INVALID_SOCKET_FD;
    m_fds_thread[0] = INVALID_SOCKET_FD;
    m_fds_thread[1] = INVALID_SOCKET_FD;
}
PipeFds::~PipeFds()
{
    ClosePipeFds(m_fds_thread);
    ClosePipeFds(m_fds_master);
}
bool PipeFds::Init()
{
    if (pipe2(m_fds_master, O_NONBLOCK | O_CLOEXEC) != 0)
    {
        Error("pipe2 m_fs_master failed, error:{}", strerror(errno));
        return false;
    }
    if (pipe2(m_fds_thread, O_NONBLOCK | O_CLOEXEC) != 0)
    {
        ClosePipeFds(m_fds_master);
        Error("pipe2 m_fs_thread failed, error:{}", strerror(errno));
        return false;
    }
    return true;
}

void PipeFds::ClosePipeFds(int fds[])
{
    if (fds[0] > INVALID_SOCKET_FD)
    {
        close(fds[0]);
        fds[0] = INVALID_SOCKET_FD;
    }
    if (fds[1] > INVALID_SOCKET_FD)
    {
        close(fds[1]);
        fds[1] = INVALID_SOCKET_FD;
    }
}