#ifndef CAR_TCP_CLIENT_H
#define CAR_TCP_CLIENT_H

#include <iostream>
#include "TcpClient.h"

#include "../jt1078/AV_Common_Define.h"
#include "../core/ipc_packet/IpcPktDecoder.h"
#include "./AsioTimer.h"

class CarTcpClient : public TcpClient
{
    typedef std::function<void(device_id_t device_id, const char *data, size_t length)> FuncNotify;

public:
    CarTcpClient(asio::io_context &io_context, const std::string &host, const std::string &port)
        : TcpClient(io_context, host, port),
          m_timer(io_context)
    {
        m_timer.SetFuncTimer(std::bind(&CarTcpClient::OnTimerReconnectJt1078Server, this));
    }
    virtual ~CarTcpClient() override {}

public:
    void SetFuncNotify(FuncNotify func) { m_func_notify = func; }

    // 发送请求，订阅对应车辆的信息
    void SendSubscribeRequest(const std::string &strDeviceId);
    // 发送请求，取消订阅对应车辆信息
    void SendUnSubscribeRequest(const std::string &strDeviceId);

protected:
    virtual void HandlerReadBuffer(const char *data, std::size_t length) override;
    virtual void HandlerReadError(const std::error_code &ec) override;
    virtual void HandlerWriteError(const std::error_code &ec) override;
    virtual void HandlerConnect(std::error_code ec, asio::ip::tcp::endpoint endpoint) override;

private:
    int SendPacket(uint32_t mask, const char *data, std::size_t len);
    uint32_t GetNewSendIpcPktSeqId();

    void ProcessIpcPacketCompleted(const ipc::packet_t &packet);

    // 当连接断开时，进行断线重连操作
    void OnTimerReconnectJt1078Server();

private:
    FuncNotify m_func_notify;
    uint32_t m_uLastSendIpcPktSeqId;

    ipc::decoder_t m_ipc_decoder; // 进程间通信的数据包格式

    AsioTimer m_timer;
};
#endif // CAR_TCP_CLIENT_H