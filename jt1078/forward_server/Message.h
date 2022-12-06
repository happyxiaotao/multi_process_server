#ifndef FORWARD_SERVER_MESSAGE_H
#define FORWARD_SERVER_MESSAGE_H
#include "../jt1078_server/Jt1078Packet.h"
#include "../AV_Common_Define.h"
namespace forward
{
    struct Message
    {
        Message(const jt1078::packet_t &pkt, iccid_t iccid) : m_pkt(pkt), m_iccid(iccid) {}
        inline const jt1078::packet_t &GetPkt() const { return m_pkt; }
        inline const iccid_t &GetIccid() const { return m_iccid; }
        const jt1078::packet_t &m_pkt;
        const iccid_t m_iccid;
    };
} // namespace forward
#endif // FORWARD_SERVER_MESSAGE_H