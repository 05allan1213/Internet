#pragma once

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <functional>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace Server
{
    static const std::string defaultIp = "0.0.0.0";
    static const int gnum = 1024;

    enum
    {
        USAGE_ERROR = 1,
        SOCKET_ERROR,
        BIND_ERROR,
        OPEN_ERROR
    }; // 错误码

    using func_t = std::function<void(int, const std::string &, const std::string &, const std::string &)>;

    class udpServer
    {

    public:
        udpServer(const func_t &callback, const uint16_t &port, const std::string &ip = defaultIp)
            : _callback(callback), _port(port), _ip(ip), _socketfd(-1)
        {
        }
        void initServer() // 初始化服务器
        {
            // 1. 创建socket
            _socketfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (_socketfd < 0)
            {
                std::cerr << "socket error: " << errno << " : " << strerror(errno) << std::endl;
                exit(SOCKET_ERROR); // 出错直接退出程序
            }
            std::cout << "socket success: " << _socketfd << std::endl;
            // 2. 绑定ip和端口(服务器要明确的port,不能随意改变)
            // 填充地址结构体
            struct sockaddr_in local;
            bzero(&local, sizeof(local));              // 清零
            local.sin_family = AF_INET;                // ipv4
            local.sin_port = htons(_port);             // 发送端口号
            local.sin_addr.s_addr = htonl(INADDR_ANY); // 绑定任意ip,服务器的真实写法
            // local.sin_addr.s_addr = inet_addr(_ip.c_str()); // ip地址
            int n = bind(_socketfd, (struct sockaddr *)&local, sizeof(local));
            if (n < 0)
            {
                std::cerr << "bind error: " << errno << " : " << strerror(errno) << std::endl;
                exit(BIND_ERROR);
            }
        }
        void start() // 启动服务器
        {
            char buffer[gnum];
            for (;;)
            {
                // 接收数据
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);                                                                   // 必填
                ssize_t s = recvfrom(_socketfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&peer, &len); //
                // 1. 数据是什么? 2. 谁发的?
                if (s > 0)
                {
                    buffer[s] = 0;
                    std::string clientIp = inet_ntoa(peer.sin_addr);
                    uint16_t clientPort = ntohs(peer.sin_port);
                    std::string message = buffer;
                    std::cout << clientIp << "[" << clientPort << "]# " << message << std::endl;

                    // 处理数据
                    _callback(_socketfd, clientIp, std::to_string(clientPort), message);
                }
            }
        }
        ~udpServer()
        {
        }

    private:
        std::string _ip;  // ip地址,实际上不建议指明一个IP,因为服务器需要绑定多个ip
        uint16_t _port;   // 端口号
        int _socketfd;    // socket文件描述符
        func_t _callback; // 回调函数
    };
}
