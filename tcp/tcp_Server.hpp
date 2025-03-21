#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include "log.hpp"
#include "ThreadPool.hpp"
#include "Task.hpp"
#include "error.hpp"

namespace server
{
    static const uint16_t DEFAULT_PORT = 8080;
    static const int DEFAULT_BACKLOG = 5;

    class TcpServer;
    class ThreadData
    {
    public:
        ThreadData(TcpServer *self, int socket)
            : _self(self), _socket(socket)
        {
        }

    public:
        TcpServer *_self;
        int _socket;
    };

    class TcpServer
    {
    public:
        TcpServer(const uint16_t &port = DEFAULT_PORT)
            : _listensocketfd(-1), _port(port)
        {
        }

        void initServer()
        {
            // 1. 创建套接字
            _listensocketfd = socket(AF_INET, SOCK_STREAM, 0);
            if (_listensocketfd < 0)
            {
                logMessage(FATAL, "create socket error");
                exit(SOCKET_ERROR);
            }
            logMessage(NORMAL, "create socket success: %d", _listensocketfd);

            // 2. 绑定ip 和 port
            struct sockaddr_in local;
            memset(&local, 0, sizeof(local));
            local.sin_family = AF_INET;
            local.sin_port = htons(_port);
            local.sin_addr.s_addr = INADDR_ANY;
            if (bind(_listensocketfd, (struct sockaddr *)&local, sizeof(local)) < 0)
            {
                logMessage(FATAL, "bind socket error");
                exit(BIND_ERROR);
            }
            logMessage(NORMAL, "bind socket success");

            // 3. 设置socket为监听状态
            if (listen(_listensocketfd, DEFAULT_BACKLOG) < 0)
            {
                logMessage(FATAL, "listen socket error");
                exit(LISTEN_ERROR);
            }
            logMessage(NORMAL, "listen socket success");
        }

        void start()
        {

            for (;;)
            {
                // 4. 线程池初始化
                ThreadPool<Task>::getInstance()->run();
                logMessage(NORMAL, "ThreadPool init success");

                // 4. server获取新链接
                // socketfd: 和client通信的fd
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);
                int socketfd = accept(_listensocketfd, (struct sockaddr *)&peer, &len);
                if (socketfd < 0)
                {
                    logMessage(ERROR, "accept socket error, next");
                    continue;
                }
                logMessage(NORMAL, "accept a new link success, get a new socketfd: %d", socketfd);
                // // version 1
                // // 5. 和client进行通信就用这个socket,面向字节流的,后续全是文件操作
                // serviceIO(socketfd);
                // // 6. 关闭socket
                // close(socketfd); // 对一个已经使用完毕的socket描述符，要关闭socket，否则会导致文件描述符泄漏

                // // version 2 (多进程版1.0)
                // pid_t pid = fork();
                // if (pid == 0) // 子进程
                // {
                //     close(_listensocketfd);
                //     if (fork() > 0)
                //     {
                //         exit(FORK_ERROR); // 退出子进程
                //     }
                //     // 以下就是孙子进程提供服务
                //     serviceIO(socketfd);
                //     close(socketfd);
                //     exit(0); // 孙子进程退出，成为孤儿进程，由init进程接管
                // }
                // // 父进程
                // pid_t ret = waitpid(pid, nullptr, 0);
                // if (ret > 0)
                // {
                //     std::cout << "wait success: " << ret << std::endl;
                // }

                // // version 2 (多进程版2.0) -- 使用信号，忽略子进程退出
                // signal(SIGCHLD, SIG_IGN);
                // pid_t pid = fork();
                // if (pid == 0) // 子进程
                // {
                //     close(_listensocketfd);
                //     serviceIO(socketfd);
                //     close(socketfd); // 释放子进程中的连接套接字资源
                //     exit(0);
                // }
                // close(socketfd); // 避免父进程保留对连接套接字的引用

                // // version 3 (多线程版)
                // pthread_t tid;
                // ThreadData *td = new ThreadData(this, socketfd);
                // pthread_create(&tid, nullptr, threadRoutine, td);

                // version 4 (线程池版)
                ThreadPool<Task>::getInstance()->Push(Task(socketfd, serviceIO));
            }
        }

        // static void *threadRoutine(void *args)
        // {
        //     pthread_detach(pthread_self());
        //     ThreadData *td = static_cast<ThreadData *>(args);
        //     td->_self->serviceIO(td->_socket);
        //     close(td->_socket);
        //     delete td;
        //     return nullptr;
        // }

        // void serviceIO(int socket)
        // {
        //     char buffer[1024];
        //     while (true)
        //     {
        //         ssize_t n = read(socket, buffer, sizeof(buffer) - 1);
        //         if (n > 0)
        //         {
        //             buffer[n] = 0;
        //             std::cout << "receive message: " << buffer << std::endl;

        //             std::string outbuffer = buffer;
        //             outbuffer += " | server[echo]";

        //             write(socket, outbuffer.c_str(), outbuffer.size());
        //         }
        //         else if (n == 0) // 代表client退出
        //         {
        //             logMessage(NORMAL, "client quit, me too");
        //             break;
        //         }
        //     }
        // }

        ~TcpServer()
        {
        }

    private:
        int _listensocketfd; // 不是用来进行数据通信的，而是用来获取新链接的
        uint16_t _port;
    };

}