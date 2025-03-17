#pragma once

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <functional> // 包含 functional 头文件以使用 std::function
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace Server
{
    static const std::string defaultIp = "0.0.0.0"; // 默认监听IP地址，0.0.0.0 表示监听所有网卡
    static const int BUFFER_SIZE = 1024;            // 定义接收缓冲区大小常量，将 gnum 更名为 BUFFER_SIZE

    // 定义错误码枚举
    enum
    {
        USAGE_ERROR = 1, // 用法错误
        SOCKET_ERROR,    // socket 创建错误
        BIND_ERROR,      // bind 错误
        OPEN_ERROR       // 文件打开错误
    };

    // 定义函数指针类型 func_t，用于回调函数，处理接收到的消息
    using func_t = std::function<void(int, const std::string &, const std::string &, const std::string &)>;
    // 参数：socket 文件描述符, 客户端 IP 地址, 客户端端口号, 接收到的消息内容

    class udpServer
    {
    public:
        // 构造函数，初始化服务器，接收回调函数、端口和IP地址
        udpServer(const func_t &callback, const uint16_t &port, const std::string &ip = defaultIp)
            : _callback(callback), _port(port), _ip(ip), _socketfd(-1)
        {
            // 构造函数体为空
        }

        // 初始化服务器 socket
        void initServer()
        {
            // 1. 创建 socket
            _socketfd = socket(AF_INET, SOCK_DGRAM, 0); // 使用 UDP 协议
            if (_socketfd < 0)
            {
                std::cerr << "[Server] socket 创建失败: " << errno << " : " << strerror(errno) << std::endl; // 错误输出到标准错误流
                exit(SOCKET_ERROR);                                                                          // 创建 socket 失败，程序退出
            }
            std::cout << "[Server] socket 创建成功: fd = " << _socketfd << std::endl; // 打印 socket 文件描述符

            // 2. 绑定 IP 地址和端口号
            struct sockaddr_in localAddr;           // 本地地址结构
            bzero(&localAddr, sizeof(localAddr));   // 初始化为 0
            localAddr.sin_family = AF_INET;         // IPv4 协议族
            localAddr.sin_port = htons(_port);      // 设置端口号，htons 用于将主机字节序转换为网络字节序
            localAddr.sin_addr.s_addr = INADDR_ANY; // 监听所有可用的网络接口，htonl(INADDR_ANY) 效果相同
            // localAddr.sin_addr.s_addr = inet_addr(_ip.c_str()); // 如果指定 IP 地址，可以使用 inet_addr 转换

            int n = bind(_socketfd, (struct sockaddr *)&localAddr, sizeof(localAddr)); // 绑定 socket
            if (n < 0)
            {
                std::cerr << "[Server] bind 失败: " << errno << " : " << strerror(errno) << std::endl; // 错误输出
                exit(BIND_ERROR);                                                                      // 绑定失败，程序退出
            }
            std::cout << "[Server] bind 成功, 监听端口: " << _port << std::endl; // 打印绑定成功信息
        }

        // 启动服务器，开始接收客户端消息
        void start()
        {
            char buffer[BUFFER_SIZE];                                           // 接收缓冲区
            std::cout << "[Server] 服务器启动，等待客户端连接..." << std::endl; // 打印服务器启动信息

            for (;;) // 无限循环，持续接收客户端消息
            {
                struct sockaddr_in peerAddr;              // 客户端地址结构
                socklen_t peerAddrLen = sizeof(peerAddr); // 客户端地址长度，recvfrom 需要使用
                bzero(&peerAddr, sizeof(peerAddr));       // 清零客户端地址结构

                // 接收客户端数据，recvfrom 会阻塞直到收到数据
                ssize_t s = recvfrom(_socketfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&peerAddr, &peerAddrLen);
                // 参数：socket 文件描述符, 接收缓冲区, 缓冲区大小, flags, 客户端地址结构, 客户端地址长度

                if (s > 0) // 接收成功
                {
                    buffer[s] = 0;                                       // 确保接收到的数据以 null 结尾，构成 C 风格字符串
                    std::string clientIp = inet_ntoa(peerAddr.sin_addr); // 将网络字节序 IP 地址转换为点分十进制字符串
                    uint16_t clientPort = ntohs(peerAddr.sin_port);      // 将网络字节序端口号转换为主机字节序

                    std::cout << "\n[Receive Message] 来自客户端 " << clientIp << ":" << clientPort << std::endl; // 打印接收消息的客户端信息
                    std::cout << "[Message Content] " << buffer << std::endl;                                     // 打印接收到的消息内容
                    std::cout << "[Server Waiting]# ";                                                            // 显示服务器等待命令提示符
                    fflush(stdout);                                                                               // 刷新标准输出，确保提示符立即显示

                    // 调用回调函数处理接收到的消息
                    _callback(_socketfd, clientIp, std::to_string(clientPort), buffer);
                }
                else if (s == 0)
                {
                    std::cout << "[Server] 客户端 " << inet_ntoa(peerAddr.sin_addr) << ":" << ntohs(peerAddr.sin_port) << " 断开连接" << std::endl; // 提示客户端断开
                }
                else
                {
                    std::cerr << "[Server] 接收消息失败: " << strerror(errno) << std::endl; // 错误输出
                    continue;                                                               // 接收出错，继续循环等待下一次接收
                }
            }
        }

        // 析构函数
        ~udpServer()
        {
            if (_socketfd != -1)
            {
                close(_socketfd);                                             // 关闭 socket 文件描述符，释放资源
                std::cout << "[Server] socket 文件描述符已关闭" << std::endl; // 打印 socket 关闭信息
            }
        }

    private:
        std::string _ip;  // 服务器监听的 IP 地址
        uint16_t _port;   // 服务器监听的端口号
        int _socketfd;    // 服务器 socket 文件描述符
        func_t _callback; // 回调函数，用于处理接收到的消息
    };
}