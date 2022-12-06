#ifndef _INI_CONFIG_H_
#define _INI_CONFIG_H_

#include <ini/INIReader.h>

extern INIReader *g_ini; // 声明为全局变量，其他模块通过extern使用，并且只读（在main函数中才能进行赋值操作）
#endif                   //_INI_CONFIG_H_
