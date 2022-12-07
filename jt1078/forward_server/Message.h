#ifndef FORWARD_SERVER_MESSAGE_H
#define FORWARD_SERVER_MESSAGE_H
#include "../jt1078_server/Jt1078Packet.h"
#include "../AV_Common_Define.h"
namespace forward
{
    struct Message
    {
        Message(const jt1078::packet_t &pkt, device_id_t device_id) : m_pkt(pkt), m_device_id(device_id) {}
        inline const jt1078::packet_t &GetPkt() const { return m_pkt; }
        inline const device_id_t &GetDeviceId() const { return m_device_id; }
        const jt1078::packet_t &m_pkt;
        const device_id_t m_device_id;
    };
} // namespace forward
#endif // FORWARD_SERVER_MESSAGE_H