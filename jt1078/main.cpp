#include "../core/eventloop/EventLoop.h"
#include "./Jt1078Service.h"
#include "../core/ini_config.h"
#include "../../core/log/Log.hpp"

INIReader *g_ini;

int main()
{
    g_ini = new INIReader("./conf/jt1078.ini");

    spdlog::default_logger()->set_level(spdlog::level::trace);

    Jt1078Service service;
    if (!service.Init())
    {
        Error("service Init failed");
        return 1;
    }
    service.Start();

    return 0;
}