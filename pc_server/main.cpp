#include "../core/eventloop/EventLoop.h"
#include "../core/ini_config.h"
#include "../../core/log/Log.hpp"
#include "PcServer.h"

INIReader *g_ini;
int main()
{
    g_ini = new INIReader("./conf/pc.ini");

    spdlog::default_logger()->set_level(spdlog::level::trace);

    PcServer server;
    if (!server.Init())
    {
        Error("pc server init failed");
        return 2;
    }
    server.Start();
    return 0;
}