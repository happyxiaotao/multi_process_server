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
        int nret = snprintf(buf, sizeof(buf) - 1, "live:%lums,read bytes:%ld,write bytse:%ld,read rate:%.2lfkb/s,write rate:%.2lfkb/s",
                            duration_ms, m_read_bytes * 8, m_write_bytes * 8, GetRate(m_read_bytes, duration_ms), GetRate(m_write_bytes, duration_ms));
        buf[nret] = '\0';
        return std::string(buf, nret);
    }

    double GetRate(uint64_t bytes, uint64_t duration_ms)
    {
        if (duration_ms == 0)
        {
            return 0;
        }
        return (bytes * 8) / 1000 / (duration_ms / 1000.0); // bytes->kbit  ms->second
    }

private:
    uint64_t m_start_time_ms;
    size_t m_read_bytes;
    size_t m_write_bytes;
};

#endif // TRAFFIC_STAT_H