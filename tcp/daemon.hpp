#pragma once

#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "error.hpp"

#define DEV "/dev/null"
void daemonSelf(const char *path = nullptr)
{
    // 1. 让调用进程忽略掉异常的信号
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    // 2. 让自己不是组长
    if (fork() > 0)
    {
        exit(FORK_ERROR);
    }
    // 子进程 -- 守护进程，精灵进程，本质是孤儿进程的一种
    pid_t pid = setsid();
    assert(pid != -1);
    // 3. 守护进程是脱离终端的，关闭或者重定向以前进程默认打开的文件描述符
    int fd = open(DEV, O_RDWR);
    if (fd >= 0)
    {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);

        close(fd);
    }
    else
    {
        close(0);
        close(1);
        close(2);
    }
    // 4. 可选：进程执行路径发生改变
    if (path)
    {
        chdir(path);
    }
}