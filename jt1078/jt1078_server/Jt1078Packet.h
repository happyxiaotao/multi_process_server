#ifndef JT1078_PACKET_H
#define JT1078_PACKET_H

#include <memory>
#include "./Jt1078Header.h"
namespace jt1078
{
    struct packet_t;
    typedef std::shared_ptr<packet_t> packet_ptr;
#pragma pack(1)
    struct packet_t
    {
        packet_t() : m_header(nullptr), m_body(nullptr)
        {
        }
        packet_t(const packet_t &packet) : m_header(nullptr), m_body(nullptr)
        {
            m_header = new header_t();
            memcpy((char *)m_header, (char *)packet.m_header, sizeof(header_t));
            m_body = new char[m_header->WdBodyLen + 1];
            memcpy(m_body, packet.m_body, m_header->WdBodyLen);
            m_body[m_header->WdBodyLen] = '\0';
        }
        ~packet_t()
        {
            if (m_header)
            {
                delete m_header;
            }
            if (m_body)
            {
                delete[] m_body;
            }
        }
        void Clear()
        {
            if (m_header)
            {
                m_header->Clear();
            }
            if (m_body)
            {
                m_body[0] = '\0';
            }
        }
        header_t *m_header;
        char *m_body;
    };
#pragma pack()
} // namespace jt1078

#endif // JT1078_PACKET_H