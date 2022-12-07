#ifndef VIDEO_SERVER_COMMON_DEFINE_H
#define VIDEO_SERVER_COMMON_DEFINE_H

#include <string>
typedef uint64_t device_id_t;
constexpr device_id_t INVALID_DEVICE_ID = 0;
namespace forward
{
    typedef device_id_t channel_id_t;
    constexpr channel_id_t INVALID_CHANNEL_ID = 0;
} // namespace forward

#endif // VIDEO_SERVER_COMMON_DEFINE_H