#ifndef TRAFFIC_STAT_H
#define TRAFFIC_STAT_H
// 流量统计

#include <string>
#include "../time/TimeUtil.h"
class TrafficStat
{
public:
    TrafficStat() : m_start_time_ms(0), m_read_bytes(0), m_write_bytes(0) {}
    ~TrafficStat() {}

public:
    inline void Start() { m_start_time_ms = GetCurrentTimeMs(); }
    inline void AddReadByte(size_t n) { m_read_bytes += n; }
    inline void AddWriteByte(size_t n) { m_write_bytes += n; }
    std::string Dump()
    {
        uint64_t current_ms = GetCurrentTimeMs();
        uint64_t duration_ms = current_ms - m_start_time_ms;
        // 单位换算成 kb/s
        char buf[128];
        int nret = snprintf(buf, sizeof(buf) - 1,
                            "live duration:%.2lf(second),read:%ld(byte),write:%ld(byte),read rate:%.2lf(kbyte/s),write rate:%.2lf(kbyte/s)",
                            duration_ms / 1000.0, m_read_bytes, m_write_bytes, GetRate_KBytePS(m_read_bytes, duration_ms), GetRate_KBytePS(m_write_bytes, duration_ms));
        buf[nret] = '\0';
        return std::string(buf, nret);
    }

    // 获取KBytes/S的值   = bytes / second
    double GetRate_KBytePS(uint64_t byte, double duration_ms)
    {
        if (duration_ms == 0)
        {
            return 0;
        }
        double kbyte = byte / 1024.0;
        double s = duration_ms / 1000.0;
        double kbyteps = kbyte / s;
        return kbyteps;
    }

private:
    uint64_t m_start_time_ms;
    size_t m_read_bytes;
    size_t m_write_bytes;
};

#endif // TRAFFIC_STAT_H