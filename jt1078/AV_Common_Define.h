#ifndef VIDEO_SERVER_COMMON_DEFINE_H
#define VIDEO_SERVER_COMMON_DEFINE_H

#include <string>
typedef uint64_t iccid_t;
constexpr iccid_t INVALID_ICCID = -1;
namespace forward
{
    typedef iccid_t channel_id_t;
    constexpr channel_id_t INVALID_CHANNEL_ID = -1;
} // namespace forward

#endif // VIDEO_SERVER_COMMON_DEFINE_H