
#include <unistd.h>
#include <assert.h>
#include "Buffer.h"
#include "../socket/Socket.h"
#include "../../core/log/Log.hpp"

Buffer::Buffer()
    : m_recv_buffer(nullptr), m_read_index(0)
{
    m_recv_buffer = new char[MAX_SIZE_ONCE_READ + 1];
    m_recv_buffer[0] = '\0';
    m_buffer.reserve(MAX_SIZE_ONCE_READ);
}
Buffer::Buffer(const Buffer &buffer)
    : m_buffer(buffer.GetBuffer(), buffer.ReadableBytes()), m_read_index(0)
{
    m_recv_buffer = new char[MAX_SIZE_ONCE_READ + 1];
    m_recv_buffer[0] = '\0';
}
Buffer::Buffer(Buffer &&buffer)
    : m_buffer(buffer.GetBuffer(), buffer.ReadableBytes()), m_read_index(0)
{
    m_recv_buffer = new char[MAX_SIZE_ONCE_READ + 1];
    m_recv_buffer[0] = '\0';
    std::string str;
    buffer.m_buffer.swap(str);
    buffer.m_read_index = 0;
}
Buffer::~Buffer()
{
    if (m_recv_buffer != nullptr)
    {
        delete[] m_recv_buffer;
        m_recv_buffer = nullptr;
    }
}
void Buffer::Clear()
{
    m_buffer.clear();
    m_read_index = 0;
}

ssize_t Buffer::GetDataFromFd(int fd, int max_size)
{
    if (fd < 0 || max_size < 0)
    {
        return -1;
    }
    int ntoread = std::min<int>(max_size, MAX_SIZE_ONCE_READ);
    int nerrno = 0;
    int nread = 0;
    do
    {
        nread = read(fd, m_recv_buffer, ntoread);
        nerrno = errno;
    } while (nread == -1 && nerrno == EINTR); //读取中断，需要再次读取

    if (nread <= 0)
    {
        Error("Buffer::GetDataFromFd, read data from fd:{},return:{},errno:{},errmsg:{}", fd, nread, nerrno, strerror(nerrno));
    }
    // if (nread == -1 && (nerrno == EAGAIN || nerrno == EWOULDBLOCK)) // 当前操作阻塞了，等待下此读取
    // {
    //     return 0;
    // }
    if (nread > 0)
    {
        // m_recv_buffer[nread] = '\0';
        m_buffer.append(m_recv_buffer, nread);
    }
    return nread;
}

int8_t Buffer::ReadInt8()
{
    return ReadInterger<int8_t>();
}
int16_t Buffer::ReadInt16()
{
    return sock::networkToHost16(ReadInterger<int16_t>());
}
int32_t Buffer::ReadInt32()
{
    return sock::networkToHost32(ReadInterger<int32_t>());
}
int64_t Buffer::ReadInt64()
{
    return sock::networkToHost64(ReadInterger<int64_t>());
}
void Buffer::ReadBuffer(char *dst, size_t n)
{
    assert(ReadableBytes() >= n);
    memcpy(dst, GetBuffer(), n);
    Skip(n);
}