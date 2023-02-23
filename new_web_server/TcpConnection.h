#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <asio.hpp>

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(asio::ip::tcp::socket socket) : m_socket(std::move(socket)) {}
    TcpConnection(asio::io_context &io_context) : m_socket(io_context) {}
    virtual ~TcpConnection() { Close(); }

public:
    void SetSocket(asio::ip::tcp::socket socket) { m_socket = std::move(socket); }
    const asio::ip::tcp::socket &GetSocketRefConst() const
    {
        return m_socket;
    }
    asio::ip::tcp::socket &GetSocketRefNoConst()
    {
        return m_socket;
    }

protected:
    void AsyncRead()
    {
        auto self(shared_from_this());
        asio::async_read(m_socket, asio::buffer(m_readBuffer), [this, self](std::error_code ec, std::size_t length)
                         {if (!ec)
            {
                this->HandlerReadBuffer(m_readBuffer.data(), length);// 处理读取到的数据
                this->AsyncRead();// 继续异步读取数据
            }
            else{
                this->HandlerReadError(ec); // 处理读取错误
            } });
    }
    void AsyncWrite(const std::string &data)
    {
        auto self(shared_from_this());
        asio::async_write(m_socket, asio::buffer(data),
                          [this, self](std::error_code ec, std::size_t length)
                          {
                            if (ec)
                            {
                                // 处理写入错误
                               this->HandlerWriteError(ec);
                            } });
    }
    void AsyncWrite(const char *data, size_t len)
    {
        auto self(shared_from_this());
        asio::async_write(m_socket, asio::buffer(data, len),
                          [this, self](std::error_code ec, std::size_t length)
                          {
                            if (ec)
                            {
                                // 处理写入错误
                               this->HandlerWriteError(ec);
                            } });
    }

    void Close()
    {
        if (m_socket.is_open())
        {
            m_socket.close();
        }
    }

protected:
    virtual void HandlerReadBuffer(const char *data, std::size_t length) = 0;
    virtual void HandlerReadError(const std::error_code &ec) = 0;
    virtual void HandlerWriteError(const std::error_code &ec) = 0;

private:
    asio::ip::tcp::socket m_socket;
    std::array<char, 1024> m_readBuffer;
};

#endif // TCP_CONNECTION_H