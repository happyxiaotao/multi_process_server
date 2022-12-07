#include "Jt1078Server.h"
#include "../../core/log/Log.hpp"
#include "../../core/socket/Socket.h"
#include "../../core/eventloop/EventLoop.h"
#include "../Jt1078Service.h"

Jt1078Server::Jt1078Server(EventLoop *eventloop, Jt1078Service *service)
    : m_listen_port(INVALID_PORT), m_eventloop(eventloop), m_service(service)
{
    m_listener = std::make_shared<Listener>(m_eventloop);
    m_listener->SetConnectionCallBack(std::bind(&Jt1078Server::OnNewConnection, this, std::placeholders::_1, std::placeholders::_2));
}

Jt1078Server::~Jt1078Server()
{
}

int Jt1078Server::Listen(const std::string &ip, u_short port)
{
    Info("Jt1078Server::Listen,ip={},port={}", ip, port);
    m_listen_ip = ip;
    m_listen_port = port;
    return m_listener->Listen(ip, port);
}

void Jt1078Server::OnNewConnection(evutil_socket_t socket, struct sockaddr *sa)
{
    std::string ip;
    u_short port;
    sock::GetIpPortFromSockaddr(*(struct sockaddr_in *)(sa), ip, port);
    Info("Jt1078Server::OnNewConnection, remote_ip={},remote_port={},fd={}", ip, port, socket);

    auto car = std::make_shared<CarSession>();
    if (!car->Init(m_eventloop, socket, ip, port))
    {
        Error("Jt1078Server::OnNewConnection failed, car init failed, remote_ip:{},remote_port:{},fd:{}", ip, port, socket);
        return;
    }
    car->SetPacketComplete(std::bind(&Jt1078Server::OnPacketCompleted, this, std::placeholders::_1, std::placeholders::_2));
    car->SetPacketError(std::bind(&Jt1078Server::OnPacketError, this, std::placeholders::_1, std::placeholders::_2));

    m_manager.AddCar(car);
}

void Jt1078Server::OnPacketError(const CarSessionPtr &car, TcpErrorType error_type)
{
    CarDisconnectCause cause = CarDisconnectCause::OtherCause;
    switch (error_type)
    {
    case TcpErrorType::TCP_ERROR_DISCONNECT:
        cause = CarDisconnectCause::RemoteClose;
        break;
    case TcpErrorType::TCP_ERROR_INVALID_PACKET:
        cause = CarDisconnectCause::InvalidJt1078Pkt;
        break;
    case TcpErrorType::TCP_ERROR_NET_ERROR:
    case TcpErrorType::TCP_ERROR_USER_BUFFER_FULL:
        cause = CarDisconnectCause::ReadError;
    default:
        break;
    }
    m_manager.DelCar((CarSessionPtr &)(car), cause);
}
void Jt1078Server::OnPacketCompleted(const CarSessionPtr &car, const jt1078::packet_t &pkt)
{
    car->UpdateDeviceIdIfEmpty(pkt.m_header->BCDSIMCardNumber, pkt.m_header->Bt1LogicChannelNumber);
    device_id_t device_id= car->GetDeviceId();
    // 当接受到完整数据包之后，需要通知主服务筋进行处理（比如转发）
    m_service->Notify1078Packet(device_id, pkt);
}
