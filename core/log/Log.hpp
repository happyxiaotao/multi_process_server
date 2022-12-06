#ifndef SWORD_LOG_H_
#define SWORD_LOG_H_
#ifndef SPDLOG_COMPILED_LIB
#define SPDLOG_COMPILED_LIB
#endif // SPDLOG_COMPILED_LIB // 使用spdlog的静态库
#include <spdlog/spdlog.h>

// 日志相关设置
bool LogConfigBeforeAll();
void LogConfigAfterAll();

// 保存进程pid，必须在daemon操作之后，否则进程id保存的是父进程id却不存在
void SaveProgPid();

#define Trace spdlog::trace
#define Debug spdlog::debug
#define Info spdlog::info
#define Warn spdlog::warn
#define Error spdlog::error

#endif // SWORD_LOG_H_