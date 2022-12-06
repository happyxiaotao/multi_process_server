#ifndef SWORD_BUFFER_H
#define SWORD_BUFFER_H
#include <string>
#include <memory>
#include <string.h>
#include <assert.h>
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
class Buffer
{
    enum : int
    {
        MAX_SIZE_ONCE_READ = 5 * 1024, //一次最多读取5k
    };

public:
    Buffer();
    Buffer(const Buffer &buffer);
    Buffer(Buffer &&buffer);
    ~Buffer();

public:
    inline const char *GetBuffer() const { return m_buffer.c_str() + m_read_index; }
    void Clear();
    inline void Remove(size_t pos = 0, size_t n = std::string::npos) { m_buffer.erase(m_read_index + pos, n); }
    inline void AutoRemove()
    {
        m_buffer.erase(0, m_read_index); //删除已经读取了的数据
        m_read_index = 0;                //重置m_read_index
    }
    inline void Append(const char *data, size_t len) { m_buffer.append(data, len); }
    inline void Append(const std::string &str) { m_buffer.append(str); }
    inline void Append(const Buffer &buffer) { m_buffer.append(buffer.GetBuffer(), buffer.ReadableBytes()); }
    inline void Reserve(size_t size) { m_buffer.reserve(size); }
    inline size_t GetCapacity() { return m_buffer.capacity(); }                       // 获取开辟的空间大小
    inline size_t GetLeftCapacity() { return m_buffer.capacity() - m_buffer.size(); } // 获取剩余空闲空间

    // 从fd中读取数据，减少一次拷贝
    ssize_t GetDataFromFd(int fd, int max_size = MAX_SIZE_ONCE_READ);

    // 从buffer中读取指定大小数据
    // 可读取数据块大小
    inline size_t ReadableBytes() const
    {
        return m_buffer.size() - m_read_index;
    }

    int8_t ReadInt8();
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();

    void ReadBuffer(char *dst, size_t n);
    inline void Skip(size_t n) { m_read_index += n; }

private:
    template <typename T>
    T ReadInterger()
    {
        assert(ReadableBytes() >= sizeof(T));

        const char *data = GetBuffer();
        T v;
        memcpy(&v, data, sizeof(v));
        Skip(sizeof v);
        return v;
    }

private:
    std::string m_buffer;
    char *m_recv_buffer;

    size_t m_read_index; //当前读取到的小标位置
};

#endif // SWORD_BUFFER_H