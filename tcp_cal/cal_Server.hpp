#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include "log.hpp"
#include "error.hpp"
#include "protocol.hpp"

namespace server
{
    static const uint16_t DEFAULT_PORT = 8080;
    static const int DEFAULT_BACKLOG = 5;

    // const Request &req：输入型
    // Response &resp：输出型
    using func_t = std::function<bool(const Request &req, Response &resp)>;

    void handlerEntery(int socketfd, func_t func)
    {
        std::string inbuffer;
        while (true)
        {
            // 1. 读取:"content_len"\r\n"x op y"\r\n
            // 1.1 如何保证读到的消息是 【一个】 完整的请求
            std::string req_text, req_str;
            // 1.2 此时这里req_text中包含了一个完整的请求
            if (!recvPackage(socketfd, inbuffer, &req_text))
            {
                return;
            }
            std::cout << "带报头的请求：\n"
                      << req_text << std::endl;
            if (!deLength(req_text, &req_str))
            {
                return;
            }
            std::cout << "去报头的正文：\n"
                      << req_str << std::endl;

            // 2. 对请求进行反序列化
            // 2.1 得到一个结构化的请求对象
            Request req;
            if (!req.deserialize(req_str))
                return;

            // 3. 计算处理  --- 业务逻辑
            // 3.1 得到一个结构化的响应对象
            Response resp;
            func(req, resp);

            // 4. 对响应进行序列化
            // 4.1 得到一个“字符串”
            std::string resp_str;
            resp.serialize(&resp_str);

            std::cout << "计算完成，序列化响应：" << resp_str << std::endl;
            // 5. 发送响应
            // 5.1 构建成一个完整的报文
            std::string send_string = enLength(resp_str);
            send(socketfd, send_string.c_str(), send_string.size(), 0);
            std::cout << " 构建完成完整的响应\n"
                      << send_string << std::endl;
        }
    }

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

        void start(func_t func)
        {

            for (;;)
            {
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

                // version 2 (多进程版1.0)
                pid_t pid = fork();
                if (pid == 0) // 子进程
                {
                    close(_listensocketfd);
                    handlerEntery(socketfd, func);
                    close(socketfd);
                    exit(0);
                }
                close(socketfd);

                // 父进程
                pid_t ret = waitpid(pid, nullptr, 0);
                if (ret > 0)
                {
                    logMessage(NORMAL, "wait child success");
                }
            }
        }

        ~TcpServer()
        {
        }

    private:
        int _listensocketfd; // 不是用来进行数据通信的，而是用来获取新链接的
        uint16_t _port;
    };

}