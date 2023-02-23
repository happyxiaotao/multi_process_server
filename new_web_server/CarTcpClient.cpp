#include "CarTcpClient.h"
#include "../core/log/Log.hpp"

void CarTcpClient::SendSubscribeRequest(const std::string &strDeviceId)
{
    SendPacket(ipc::IPC_PKT_TYPE_SUBSCRIBE_DEVICE_ID, strDeviceId.c_str(), strDeviceId.size());
}

void CarTcpClient::SendUnSubscribeRequest(const std::string &strDeviceId)
{
    SendPacket(ipc::IPC_PKT_TYPE_UNSUBSCRIBE_DEVICE_ID, strDeviceId.c_str(), strDeviceId.size());
}

int CarTcpClient::SendPacket(uint32_t mask, const char *data, std::size_t len)
{
    ipc::packet_t pkt;
    pkt.m_uPktType = mask;
    pkt.m_uDataLength = len;
    pkt.m_uPktSeqId = GetNewSendIpcPktSeqId();
    struct iovec iov[2];
    iov[0].iov_base = &pkt;
    iov[0].iov_len = sizeof(ipc::packet_t);
    iov[1].iov_base = (void *)data;
    iov[1].iov_len = len;

    AsyncWrite((char *)iov[0].iov_base, iov[0].iov_len);
    AsyncWrite((char *)iov[1].iov_base, iov[1].iov_len);

    return 0;
}

uint32_t CarTcpClient::GetNewSendIpcPktSeqId()
{
    if (m_uLastSendIpcPktSeqId == ipc::INVALID_PKT_SEQ_ID)
    {
        m_uLastSendIpcPktSeqId = 0;
    }
    else
    {
        ++m_uLastSendIpcPktSeqId;
    }
    return m_uLastSendIpcPktSeqId;
}

void CarTcpClient::ProcessIpcPacketCompleted(const ipc::packet_t &packet)
{
    uint32_t uIpcPktType = packet.m_uPktType & ipc::IPC_PKT_TYPE_MASK;

    switch (uIpcPktType)
    {
    // 订阅通道
    case ipc::IPC_PKT_TYPE_SUBSCRIBE_DEVICE_ID:
    {
        break;
    }
    // 停止订阅。
    case ipc::IPC_PKT_TYPE_UNSUBSCRIBE_DEVICE_ID:
    {
        break;
    }
    // 处理jt1078音视频数据
    case ipc::IPC_PKT_TYPE_JT1078_PACKET:
    {
        const char *p = packet.m_data;
        // 获取
        device_id_t *_device_id = (device_id_t *)p;
        p += sizeof(device_id_t);
        size_t len = packet.m_uDataLength - sizeof(device_id_t);
        if (m_func_notify)
        {
            m_func_notify(*_device_id, p, len);
        }

        break;
    }

    default:
        break;
    }
}

void CarTcpClient::OnTimerReconnectJt1078Server()
{
    Trace("CarTcpClient::OnTimerReconnectJt1078Server");
    ReConnect();
}

void CarTcpClient::HandlerReadBuffer(const char *data, std::size_t length)
{
    auto error = m_ipc_decoder.PushBuffer(data, length);
    if (error == ipc::decoder_t::kBufferFull)
    {
        Error("CarTcpClient::HandlerReadBuffer, error_type:{}", error);
        // ProcessIpcPacketError();
        return;
    }
    while ((error = m_ipc_decoder.Decode()) == ipc::decoder_t::kNoError)
    {
        const auto &packet = m_ipc_decoder.GetPacket();
        ProcessIpcPacketCompleted(packet);
    }
    if (error != ipc::decoder_t::kNeedMoreData)
    {
        // ProcessIpcPacketError();
        Error("CarTcpClient::HandlerReadBuffer, error_type:{}", error);
        return;
    }
}

void CarTcpClient::HandlerReadError(const std::error_code &ec)
{
    Error("CarTcpClient::HandlerReadError,error_code:{},error_msg:{}", ec.value(), ec.message());
    Close();
    m_timer.StartTimer(5);
}

void CarTcpClient::HandlerWriteError(const std::error_code &ec)
{
    Error("CarTcpClient::HandlerWriteError,error_code:{},error_msg:{}", ec.value(), ec.message());
    Close();
    m_timer.StartTimer(5);
}

void CarTcpClient::HandlerConnect(std::error_code ec, asio::ip::tcp::endpoint endpoint)
{
    if (ec)
    {
        Error("CarTcpClient::HandlerConnect failed, host:{},port:{},error_code:{},error_msg:{}", GetHost(), GetPort(), ec.value(), ec.message());

        // 5秒后重连
        m_timer.StartTimer(5);

        return;
    }
    AsyncRead();
}
