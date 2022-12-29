#ifndef JT1078_CLIENT_H
#define JT1078_CLIENT_H
#include "../core/tcp/TcpClient.h"
#include "../core/ipc_packet/IpcPktDecoder.h"
class Jt1078Client;
typedef std::shared_ptr<Jt1078Client> Jt1078ClientPtr;
class Jt1078Client : public TcpClient
{
    typedef std::function<void(const Jt1078ClientPtr &client, bool bOk)> FunctorOnConnect;
    typedef std::function<void(const Jt1078ClientPtr &client, const ipc::packet_t &packet)> FunctorOnMessage;
    typedef std::function<void(const Jt1078ClientPtr &client, TcpErrorType error)> FunctorOnError;

public:
    Jt1078Client();
    virtual ~Jt1078Client();

public:
    void SetHandlerOnConnect(FunctorOnConnect handler) { m_functor_connect = handler; }
    void SetHandlerOnMessage(FunctorOnMessage handler) { m_functor_message = handler; }
    void SetHandlerOnError(FunctorOnError handler) { m_functor_error = handler; }

public:
    int SendPacket(ipc::IpcPktType type, const char *data, size_t len);
    inline uint32_t GetLastReceiveIpcPktSeqId() { return m_uLastReceiveIpcPktSeqId; }

private:
    virtual void OnConnected(evutil_socket_t fd, bool bOk, const char *err) override;
    virtual void OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer) override;
    virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) override;

private:
    uint32_t GetNewSendIpcPktSeqId();

private:
    ipc::decoder_t m_ipc_decoder;
    ipc::packet_t m_ipc_packet;

    FunctorOnConnect m_functor_connect;
    FunctorOnMessage m_functor_message;
    FunctorOnError m_functor_error;

    uint32_t m_uLastReceiveIpcPktSeqId; //最近接收的数据包序号
    uint32_t m_uLastSendIpcPktSeqId;    //最近发送的数据包序号
};
#endif // JT1078_CLIENT_H