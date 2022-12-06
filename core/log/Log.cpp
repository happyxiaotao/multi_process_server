
#include "./Log.hpp"
#include "../ini_config.h"
#include <spdlog/sinks/rotating_file_sink.h> // 循环日志文件
#include <spdlog/sinks/basic_file_sink.h>    // 固定日志文件
#include <spdlog/sinks/stdout_color_sinks.h> // 控制台输出
#include <spdlog/async.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

bool LogConfigBeforeAll()
{
    std::shared_ptr<spdlog::logger> logger;

    const std::string strDir = g_ini->Get("log", "dir", "./logs");
    if (strDir.empty())
    {
        Error("empty log dir");
        return false;
    }
    const std::string strFileName = g_ini->Get("log", "log_file", "card.log");
    const std::string strFilePath = strDir + "/" + strFileName;

    const std::string strMode = g_ini->Get("log", "mode", "console");
    if (strMode == "console")
    {
        logger = spdlog::stdout_color_mt<spdlog::async_factory>("console");
    }
    else if (strMode == "basic")
    {
        if (strFileName.empty())
        {
            fprintf(stderr, "empty log filename!");
            return false;
        }
        logger = spdlog::basic_logger_mt("basic", strFilePath, false);
    }
    else if (strMode == "rotating")
    {
        if (strFileName.empty())
        {
            fprintf(stderr, "empty log filename!");
            return false;
        }
        int nMaxFileSize = g_ini->GetInteger("log", "max_file_size", 1024 * 1024);
        int nMaxFiles = g_ini->GetInteger("log", "max_files", 10);
        logger = spdlog::rotating_logger_mt("rotating", strFilePath, nMaxFileSize, nMaxFiles);
    }
    else
    {
        fprintf(stderr, "unknwon mode type!");
        return false;
    }
    if (logger == nullptr)
    {
        fprintf(stderr, "allocate logger failed!\n");
        return false;
    }
    const std::string strLevel = g_ini->Get("log", "level", "info");
    if (strLevel == "trace")
    {
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
    }
    else if (strLevel == "debug")
    {
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::debug);
    }
    else if (strLevel == "info")
    {
        logger->set_level(spdlog::level::info);
    }
    else if (strLevel == "warn")
    {
        logger->set_level(spdlog::level::warn);
    }
    else if (strLevel == "critical")
    {
        logger->set_level(spdlog::level::critical);
    }
    else
    {
        fprintf(stderr, "unknwon level type!");
        return false;
    }
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][pid:%P][tid:%t][%l] %v");
    // logger->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(logger);

    return true;
}

void LogConfigAfterAll()
{
    spdlog::shutdown();
}

void SaveProgPid()
{
    const std::string strDir = g_ini->Get("log", "dir", "./logs");
    if (access(strDir.c_str(), F_OK) != 0)
    {
        Info("{} not exists, create it!", strDir);
        int n = mkdir(strDir.c_str(), 0775);
        if (n != 0)
        {
            Error("create dir {} failed, error:{}", strDir, strerror(errno));
        }
    }
    const std::string strPidFileName = g_ini->Get("log", "pid_file", "card.pid");
    if (strPidFileName.empty())
    {
        return;
    }
    std::string strPidFilePath = strDir + "/" + strPidFileName;
    FILE *fp = fopen(strPidFilePath.c_str(), "w");
    if (fp == nullptr)
    {
        Error("SaveProgPid, create pid_file:{} failed, error:{}", strPidFilePath, strerror(errno));
        return;
    }
    const std::string strPid = std::to_string(getpid());
    fwrite(strPid.c_str(), 1, strPid.size(), fp);
    fclose(fp);
    fp = nullptr;
}
