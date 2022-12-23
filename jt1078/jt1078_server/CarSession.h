#ifndef JT1078_SERVER_CAR_SESSION_H
#define JT1078_SERVER_CAR_SESSION_H

#include <string>
#include "Jt1078Decoder.h"
#include "../../core/tcp/TcpSession.h"
#include "../../core/traffic/TrafficStat.h"
#include "../AV_Common_Define.h"

enum class CarDisconnectCause : int
{
    UnknownCause = 0,
    RemoteClose = 1,         // 对端关闭
    ReadError = 2,           // read调用返回-1
    InvalidJt1078Pkt = 3,    // 不合理的jt1078包
    InvalidDeviceId = 4,     // 不对的deviceId
    FirstReceiveTimeout = 5, // 第一次读取数据超时（避免异常连接占用资源）
    HandlerError = 6,        // 消息处理失败
    WebClose = 7,            // 前段关闭了网页
    SrsPublishError = 8,     // srs推流失败，可能srs断开或推流数据异常
    KickCar = 9,             // 互踢，存在两个相同的device_id,会踢掉之前的，目前未实现
    NoSubscriber = 10,       // 没有订阅者，则不需要维持此连接了
    OtherCause = 11,         // 其他清空（目前还未发现使用的地方）
};

class CarSession;
typedef std::shared_ptr<CarSession> CarSessionPtr;
class CarSession : public TcpSession
{
public:
    CarSession();
    virtual ~CarSession() override;

    typedef std::function<void(const CarSessionPtr &car, const jt1078::packet_t &pkt)> FunctorPacketComplete;
    typedef std::function<void(const CarSessionPtr &car, TcpErrorType error)> FunctorOnError;

public:
    inline const std::string &GetDeviceIdStr() const { return m_strDeviceId; }
    inline device_id_t GetDeviceId() const { return m_device_id; }
    inline void SetDisconnectCause(CarDisconnectCause cause) { m_disconnect_cause = cause; }

    inline void SetPacketComplete(const FunctorPacketComplete &functor) { m_functor_complete = functor; }
    inline void SetPacketError(const FunctorOnError &functor) { m_functor_error = functor; }

    void UpdateDeviceIdIfEmpty(const uint8_t *pSimCardNumber, uint8_t uLogicChannelNumber);

private:
    virtual void OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer) override;
    virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) override;

private:
    void ParseBuffer(const Buffer &buffer);

private:
    std::string m_strDeviceId;
    device_id_t m_device_id;

    jt1078::decoder_t m_decoder;
    CarDisconnectCause m_disconnect_cause;

    FunctorPacketComplete m_functor_complete; // 一个完整包的处理函数
    FunctorOnError m_functor_error;

    TrafficStat m_traffic_stat;
};

#endif // JT1078_SERVER_CAR_SESSION_H