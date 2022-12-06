#ifndef PC_SERVER_PC_SESSION_H
#define PC_SERVER_PC_SESSION_H

#include "../core/tcp/TcpSession.h"
#include "../core/ipc_packet/IpcPktDecoder.h"
class PcSession;
typedef std::shared_ptr<PcSession> PcSessionPtr;
class PcSession : public TcpSession
{
    typedef std::function<void(const PcSessionPtr &car, const ipc::packet_t &packet)> FunctorPacketComplete;
    typedef std::function<void(const PcSessionPtr &car, TcpErrorType error)> FunctorOnError;

public:
    PcSession();
    ~PcSession();

public:
    inline void SetPacketComplete(const FunctorPacketComplete &functor) { m_functor_complete = functor; }
    inline void SetPacketError(const FunctorOnError &functor) { m_functor_error = functor; }
    inline const std::string &GetIccid() const { return m_strIccid; }
    inline void SetIccid(const std::string &strIccid) { m_strIccid = strIccid; }

    ssize_t SendPacket(const ipc::packet_t &packet);

private:
    virtual void OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer) override;
    virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) override;

    uint32_t GetNewSendIpcPktSeqId();

private:
    ipc::decoder_t m_ipc_decoder;
    uint32_t m_uLastReceiveIpcPktSeqId;
    uint32_t m_uLastSendIpcPktSeqId;
    FunctorPacketComplete m_functor_complete; // 一个完整包的处理函数
    FunctorOnError m_functor_error;

    std::string m_strIccid; //观看的iccid
};

#endif // PC_SERVER_PC_SESSION_H