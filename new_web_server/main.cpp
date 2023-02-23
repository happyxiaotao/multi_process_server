#include "CarWebSocketServer.h"
#include "CarTcpClient.h"
#include "CarWebServer.h"
#include "../core/log/Log.hpp"
#include "../core/ini_config.h"

INIReader *g_ini;

#include <unistd.h>
bool DoBeforeMainLoop(const std::string &strConfigFile)
{
    g_ini = new INIReader(strConfigFile);
    int nErrorLineNumber = g_ini->ParseError(); // 检查config.ini配置文件哪一行解析失败
    if (nErrorLineNumber == -1)
    {
        fprintf(stderr, "配置文件解析失败，文件:[%s] 不存在，程序退出!\n", strConfigFile.c_str());
        delete g_ini;
        g_ini = nullptr;
        return false;
    }
    else if (nErrorLineNumber > 0)
    {
        fprintf(stderr, "配置文件解析失败，错误行号:%d，程序退出!\n", nErrorLineNumber);
        delete g_ini;
        g_ini = nullptr;
        return false;
    }

    // 根据配置文件，进行日志操作初始化
    if (!LogConfigBeforeAll())
    {
        fprintf(stderr, "解析日志配置失败，程序退出!\n");
        delete g_ini;
        g_ini = nullptr;
        return false;
    }

    // 是否在后台执行
    bool bDaemon = g_ini->GetBoolean("core", "daemon", false);
    if (bDaemon)
    {
        if (daemon(1, 1) < 0)
        {
            fprintf(stderr, "daemon(1,1) failed!,error:%s\n", strerror(errno));
            exit(0);
        }
    }

    // 保存进程id
    SaveProgPid();
    return true;
}

#include "CarWebServerVersion.h"
void ShowUsage()
{
    std::string strUsage;
    // 程序编译时间
    strUsage.append("---------- build time:          " + std::string(__DATE__ " " __TIME__) + "\n");
    // // crypto版本
    // strUsage.append("---------- crypto version:      " + std::string(OPENSSL_VERSION_TEXT) + "\n");
    // spdlog版本
    char tmp[20];
    snprintf(tmp, sizeof(tmp), "%d.%d.%d", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    strUsage.append("----------[ spdlog version:      ]    " + std::string(tmp) + "\n");

    // asio版本号
    // ASIO_VERSION % 100 is the sub-minor version
    // ASIO_VERSION / 100 % 1000 is the minor version
    // ASIO_VERSION / 100000 is the major version
    // #define ASIO_VERSION 102400 // 1.24.0
    int nAsioMajor = ASIO_VERSION / 100000;
    int nAsioMinor = ASIO_VERSION / 100 % 1000;
    int nAsioSubMinor = ASIO_VERSION % 100;
    std::string strAsioVersion = std::to_string(nAsioMajor) + "." + std::to_string(nAsioMinor) + "." + std::to_string(nAsioSubMinor);
    strUsage.append("----------[ asio version:        ]    " + strAsioVersion + "\n");

    // websocket++版本号
    strUsage.append("----------[ websocket++ version: ]    " + std::string(websocketpp::user_agent) + "\n");

    // web server版本
    strUsage.append("----------[ server version:      ]    " + std::string(CAR_WEB_SERVER_VERSION) + "\n");
    Info("\n----------------- Server Usage Begin  ---------------------\n"
         "{}"
         "----------------- Server Usage End    ---------------------\n",
         strUsage);
}

int main(int argc, char **argv)
{
    bool bExit = false;
    const char *optstring = "f:v";
    int nOpt = -1;
    std::string strConfigFile = "./conf/new_web_server.ini";
    while ((nOpt = getopt(argc, argv, optstring)) != -1)
    {
        switch (nOpt)
        {
        case 'f':
            strConfigFile.assign(optstring);
            break;
        case 'v':
            ShowUsage();
            bExit = true;
            break;
        default:
            printf("未知参数，退出程序!\n");
            bExit = true;
            break;
        }
    }
    if (bExit)
    {
        exit(0);
    }

    if (!DoBeforeMainLoop(strConfigFile))
    {
        exit(0);
    }

    ShowUsage();

    const std::string strWebSocketListenIp = g_ini->Get("websocket", "ip", "");
    int nWebSocketListenPort = g_ini->GetInteger("websocket", "port", 0);

    const std::string strJt1078Ip = g_ini->Get("jt1078", "forward_ip", "");
    int nJt1078Port = g_ini->GetInteger("jt1078", "forward_port", 0);

    if (strWebSocketListenIp.empty() || nWebSocketListenPort == 0)
    {
        Error("invalid websocket listen ip or port, exit program!");
        exit(0);
    }
    if (strJt1078Ip.empty() || nJt1078Port == 0)
    {
        Error("invalid jt1078 ip or port, exit program!\n");
        exit(0);
    }

    asio::io_context io_context;
    CarWebServer server(io_context);

    Info("websocket listen, ip:{}, port:{}", strWebSocketListenIp, nWebSocketListenPort);
    server.Listen(strWebSocketListenIp, nWebSocketListenPort);

    Info("jt1078_client,target ip:{},port:{}", strJt1078Ip, nJt1078Port);
    server.ConnectServer(strJt1078Ip, nJt1078Port);

    io_context.run();
}