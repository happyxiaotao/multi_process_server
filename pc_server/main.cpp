#include "../core/eventloop/EventLoop.h"
#include "../core/ini_config.h"
#include "../../core/log/Log.hpp"
#include "PcServer.h"

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
        daemon(1, 1);
    }

    // 保存进程id
    SaveProgPid();
    return true;
}

#include "./PcServerVersion.h"
void ShowUsage()
{
    std::string strUsage;
    // 程序编译时间
    strUsage.append("---------- build time:          " + std::string(__DATE__ " " __TIME__) + "\n");
    // libevent版本
    strUsage.append("---------- libevent version:    " + std::string(event_get_version()) + "\n");
    // // crypto版本
    // strUsage.append("---------- crypto version:      " + std::string(OPENSSL_VERSION_TEXT) + "\n");
    // spdlog版本
    char tmp[20];
    snprintf(tmp, sizeof(tmp), "%d.%d.%d", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    strUsage.append("---------- spdlog version:      " + std::string(tmp) + "\n");
    // // mysqlclient版本
    // strUsage.append("---------- mysqlclient version: " + std::string(mysql_get_client_info()) + "\n");
    // // hiredis版本
    // snprintf(tmp, sizeof(tmp), "%d.%d.%d", HIREDIS_MAJOR, HIREDIS_MINOR, HIREDIS_PATCH);
    // strUsage.append("---------- hiredis version:     " + std::string(tmp) + "\n");
    // pc server版本
    strUsage.append("---------- server version:      " + std::string(PC_SERVER_VERSION) + "\n");
    Info("----------------- Server Usage Begin  ---------------------\n"
         "{}"
         "----------------- Server Usage End    ---------------------\n",
         strUsage);
}

int main(int argc, char **argv)
{
    bool bExit = false;
    const char *optstring = "f:v";
    int nOpt = -1;
    std::string strConfigFile = "./conf/pc.ini";
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

    PcServer server;
    if (!server.Init())
    {
        Error("pc server init failed");
        return 2;
    }
    server.Start();
    return 0;
}