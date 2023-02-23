#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "TcpConnection.h"

class TcpClient : public TcpConnection
{
public:
    TcpClient(asio::io_context &io_context, const std::string &host, const std::string &port)
        : TcpConnection(io_context),
          m_resolver(io_context),
          m_host(host),
          m_port(port)

    {
        Connect(m_host, m_port);
    }

    virtual ~TcpClient() override {}

public:
    const std::string &GetHost() { return m_host; }
    const std::string &GetPort() { return m_port; }

protected:
    // 重连
    void ReConnect()
    {
        printf("Reconnect\n");
        Close();
        Connect(m_host, m_port);
    }
    void Connect(const std::string &host, const std::string &port)
    {
        // 解析host和port
        asio::ip::tcp::resolver::results_type endpoints = m_resolver.resolve(host, port);

        // 异步连接
        asio::async_connect(GetSocketRefNoConst(), endpoints,
                            [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                            { this->HandlerConnect(ec, endpoint); });
    }

protected:
    virtual void HandlerReadBuffer(const char *data, std::size_t length) {}
    virtual void HandlerReadError(const std::error_code &ec) {}
    virtual void HandlerWriteError(const std::error_code &ec) {}

    virtual void HandlerConnect(std::error_code ec, asio::ip::tcp::endpoint endpoint) = 0;

private:
    asio::ip::tcp::resolver m_resolver;
    std::string m_host;
    std::string m_port;
};

#endif // TCP_CLIENT_H