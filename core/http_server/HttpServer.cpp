#include <openssl/ssl.h>
#include <openssl/err.h>
#include <event2/http.h>
#include <event2/bufferevent_ssl.h>
#include "../eventloop/EventLoop.h"

#include "HttpServer.h"
#include "../log/Log.hpp"
#include "../ini_config.h"

HttpServer::HttpServer() : m_http(nullptr), m_handle(nullptr), m_bSupportSSL(false)
{
}
HttpServer::~HttpServer()
{
}

bool HttpServer::SupportSSL(const std::string &strCertificateChain, const std::string &strPrivateKey)
{
    if (strCertificateChain.empty() || strPrivateKey.empty())
    {
        Error("CHttpServer::SupportSSL, cert pem or key file empty!");
        return false;
    }

    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE | SSL_OP_NO_SSLv2);
    EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!ecdh)
    {
        Error("EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)");
        return false;
    }
    if (1 != SSL_CTX_set_tmp_ecdh(ctx, ecdh))
    {
        Error("1!=SSL_CTX_set_tmp_ecdh(ctx, ecdh)");
        return false;
    }
    // 使用服务器证书和服务器私钥

    if (1 != SSL_CTX_use_certificate_chain_file(ctx, strCertificateChain.c_str()))
    {
        Error("SSL_CTX_use_certificate_chain_file error, strCertificateChain:{}", strCertificateChain);
        return false;
    }
    if (1 != SSL_CTX_use_PrivateKey_file(ctx, strPrivateKey.c_str(), SSL_FILETYPE_PEM))
    {
        Error("SSL_CTX_use_PrivateKey_file error, strPrivateKey:{}", strPrivateKey);
        return false;
    }

    if (1 != SSL_CTX_check_private_key(ctx))
    {
        Error("SSL_CTX_check_private_key error");
        return false;
    }

    evhttp_set_bevcb(m_http, ssl_bev_cb, ctx);
    m_bSupportSSL = true;
    return true;
}

bool HttpServer::Init(EventLoop *eventloop, const std::string &ip, u_short port)
{
    m_eventloop = eventloop;
    m_http = evhttp_new(eventloop->GetEventBase());
    if (m_http == nullptr)
    {
        Error("HttpServer::Init, evhttp_new return nullptr");
        return false;
    }
    if (!InitHttpHandlers())
    {
        Error("HttpServer::Init, InitHttpHandlers failed");
        return false;
    }

    // 绑定ip和port
    m_handle = evhttp_bind_socket_with_handle(m_http, ip.c_str(), port);
    if (m_handle == nullptr)
    {
        Error("HttpServer::Init, m_handler init failed, ip:{},port:{}", ip, port);
        return false;
    }

    Trace("HttpServer,listen ip:{},port:{}", ip, port);

    return true;
}

struct bufferevent *HttpServer::ssl_bev_cb(struct event_base *base, void *arg)
{
    SSL_CTX *ctx = static_cast<SSL_CTX *>(arg);
    return bufferevent_openssl_socket_new(
        base, -1, SSL_new(ctx), BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
}
