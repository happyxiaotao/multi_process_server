#include "CarSession.h"
#include "Jt1078Util.h"
#include "../../core/log/Log.hpp"

CarSession::CarSession() : m_iccid(INVALID_ICCID), m_disconnect_cause(CarDisconnectCause::UnknownCause)
{
    m_traffic_stat.Start();
}

CarSession::~CarSession()
{
    Info("CarSession::~CarSession, remote_ip:{},remote_port:{},socket:{},iccid:{:x},disconnect_case:{},traffic_stat:{}",
         GetRemoteIp(), GetRemotePort(), GetSocketFd(), m_iccid, static_cast<int>(m_disconnect_cause), m_traffic_stat.Dump());
}
void CarSession::OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer)
{
    auto self = std::dynamic_pointer_cast<CarSession>(tcp);
    // Trace("buffer:{}", buffer.GetBuffer());
    m_traffic_stat.AddReadByte(buffer.ReadableBytes());
    ParseBuffer(buffer);
}
void CarSession::OnError(const TcpSessionPtr &tcp, TcpErrorType error_type)
{
    if (m_functor_error)
    {
        auto self = std::dynamic_pointer_cast<CarSession>(tcp);
        m_functor_error(self, error_type);
    }
}

void CarSession::ParseBuffer(const Buffer &buffer)
{
    auto self = std::dynamic_pointer_cast<CarSession>(shared_from_this());
    auto error = m_decoder.PushBuffer(buffer);
    if (error != jt1078::decoder_t::kNoError)
    {
        auto e = TCP_ERROR_USER_BUFFER_FULL;
        if (m_functor_error)
        {
            OnError(self, e);
        }
        return;
    }

    while ((error = m_decoder.Decode()) == jt1078::decoder_t::kNoError)
    {
        if (m_functor_complete)
        {
            const auto &packet = m_decoder.GetPacket();
            m_functor_complete(self, packet);
        }
    }

    if (error != jt1078::decoder_t::kNeedMoreData)
    {
        Error("CarSession::ParseBuffer,m_decoder.Decode() return error:{}", error);
        auto e = TCP_ERROR_INVALID_PACKET;
        if (m_functor_error)
        {
            m_functor_error(self, e);
        }
        return;
    }
    else
    {
        //  sword::Trace("CarSession::ParseBuffer,need more buffer,buffer length:{},need length:{}", m_decoder.GetReadableBytes(), m_decoder.GetHowmuch());
    }
}
void CarSession::UpdateIccidIfEmpty(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber)
{
    if (m_iccid == INVALID_ICCID || m_strIccid.empty())
    {
        m_strIccid = GenerateIccidStr(pSimCardNumber, uLogicChannelNumber);
        m_iccid = GenerateIccid(m_strIccid);
        Debug("CarSession::UpdateIccidIfEmpty,session_id:{},iccid:{:x}", GetSessionId(), m_iccid);
    }
}