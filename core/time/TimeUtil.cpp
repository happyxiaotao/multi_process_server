#include "TimeUtil.h"
uint64_t GetCurrentTimeMs()
{
    struct timeval time;
    /* 获取时间，理论到us */
    gettimeofday(&time, NULL);
    uint64_t ms = time.tv_sec * 1000 + time.tv_usec / 1000;
    return ms;
}