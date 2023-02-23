#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <asio.hpp>

class WebSocketServer
{
public:
    WebSocketServer(asio::io_service &io_service, const asio::ip::tcp::endpoint &endpoint) : m_endpoint(endpoint)
    {
        m_server.init_asio(&io_service);

        m_server.clear_access_channels(websocketpp::log::alevel::all);
        m_server.clear_error_channels(websocketpp::log::elevel::all);
        m_server.set_access_channels(websocketpp::log::alevel::none);
        // m_server.set_access_channels(websocketpp::log::alevel::connect | websocketpp::log::alevel::disconnect);
        m_server.set_error_channels(websocketpp::log::elevel::none);
        // m_server.set_error_channels(websocketpp::log::elevel::warn);

        // Set WebSocket server configuration
        m_server.set_reuse_addr(true);
        m_server.set_message_handler(websocketpp::lib::bind(&WebSocketServer::onMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
        m_server.set_open_handler(websocketpp::lib::bind(&WebSocketServer::onOpen, this, websocketpp::lib::placeholders::_1));
        m_server.set_close_handler(websocketpp::lib::bind(&WebSocketServer::onClose, this, websocketpp::lib::placeholders::_1));
        m_server.set_fail_handler(websocketpp::lib::bind(&WebSocketServer::onError, this, websocketpp::lib::placeholders::_1));
    }
    virtual ~WebSocketServer()
    {
    }

public:
    void Start()
    {
        // Listen on the specified port
        m_server.listen(m_endpoint);

        // Start the WebSocket server
        m_server.start_accept();
    }
    void Stop()
    {
        m_server.stop();
    }

protected:
    websocketpp::server<websocketpp::config::asio> &GetServer() { return m_server; }

    void CloseConnectionHdl(websocketpp::connection_hdl hdl, websocketpp::close::status::value code, const std::string &reason)
    {
        m_server.close(hdl, code, reason);
    }

protected:
    virtual void onOpen(websocketpp::connection_hdl hdl) = 0;

    virtual void onMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg) = 0;

    virtual void onClose(websocketpp::connection_hdl hdl) = 0;

    virtual void onError(websocketpp::connection_hdl hdl) = 0;

private:
    asio::ip::tcp::endpoint m_endpoint;
    websocketpp::server<websocketpp::config::asio> m_server;
};

#endif // WEBSOCKET_H