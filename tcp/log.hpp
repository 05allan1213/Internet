#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

enum
{
    DEBUG = 0,
    NORMAL,
    WARNING,
    ERROR,
    FATAL
};

static const std::string file_name = "./log.txt";

std::string getLevel(int level)
{
    std::vector<std::string> vs = {"<DEBUG>", "<NORMAL>", "<WARNING>", "<ERROR>", "<FATAL>", "<UNKNOWN>"};

    // 避免非法情况
    if (level < 0 || level >= vs.size() - 1)
        return vs[vs.size() - 1];

    return vs[level];
}

std::string getTime()
{
    time_t t = time(nullptr);      // 获取时间戳
    struct tm *st = localtime(&t); // 获取时间相关的结构体

    char buff[128];
    snprintf(buff, sizeof(buff), "%d-%d-%d %d:%d:%d", st->tm_year + 1900, st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min, st->tm_sec);

    return buff;
}

// 处理信息
void logMessage(int level, const char *format, ...)
{
    // 日志格式：<日志等级> [时间] [PID] {消息体}
    std::string logmsg = getLevel(level);                 // 获取日志等级
    logmsg += " " + getTime();                            // 获取时间
    logmsg += " [pid: " + std::to_string(getpid()) + "]"; // 获取进程PID

    // 截获主体消息
    char msgbuff[1024];
    va_list p;
    va_start(p, format);                            // 将 p 定位至 format 的起始位置
    vsnprintf(msgbuff, sizeof(msgbuff), format, p); // 自动根据格式进行读取
    va_end(p);

    logmsg += " {" + std::string(msgbuff) + "}"; // 获取主体消息

    // 持久化。写入文件中
    FILE *fp = fopen(file_name.c_str(), "a"); // 以追加的方式写入
    if (fp == nullptr)
        return; // 不太可能出错

    fprintf(fp, "%s\n", logmsg.c_str());
    fflush(fp); // 手动刷新一下
    fclose(fp);
    fp = nullptr;
}
