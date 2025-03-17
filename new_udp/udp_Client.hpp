#pragma once

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <atomic> // 引入 atomic

namespace Client
{
    // 定义错误码枚举
    enum
    {
        USAGE_ERROR = 1, // 用法错误
        SOCKET_ERROR,    // socket 创建错误
        BIND_ERROR       // bind 错误 (虽然客户端通常不显式 bind)
    };

    class udpClient
    {
    public:
        // 构造函数，初始化服务器IP地址和端口号
        udpClient(const std::string &ip, const uint16_t &port)
            : _server_ip(ip), _server_port(port), _socketfd(-1), _quit(false)
        {
            // 构造函数体为空
        }

        // 初始化客户端
        void initClient()
        {
            // 1. 创建 socket
            _socketfd = socket(AF_INET, SOCK_DGRAM, 0); // 使用 UDP 协议
            if (_socketfd == -1)
            {
                std::cerr << "[Client] socket 创建失败: " << strerror(errno) << std::endl; // 错误输出到标准错误流
                exit(SOCKET_ERROR);                                                        // 客户端创建socket失败，程序退出
            }
            std::cout << "[Client] socket 创建成功: fd = " << _socketfd << std::endl; // 打印socket文件描述符
            // 2. 客户端通常不需要显式 bind，操作系统会自动分配一个可用的端口和IP地址
            //    写服务器的公司是一家，写客户端的公司是无数家 -- 由操作系统自动 bind
        }

        // 静态方法，用于线程中接收消息
        static void *readMessage(void *args)
        {
            int sockfd = *(static_cast<int *>((args))); // 获取 socket 文件描述符
            pthread_detach(pthread_self());             // 分离线程，线程结束后自动释放资源

            while (true)
            {
                char buffer[1024];                        // 接收缓冲区
                struct sockaddr_in tempAddr;              // 临时存储发送端地址信息
                socklen_t tempAddrLen = sizeof(tempAddr); // 地址长度
                // 接收数据，recvfrom 会阻塞直到收到数据
                ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&tempAddr, &tempAddrLen);
                if (n > 0)
                {
                    buffer[n] = 0;                                             // 确保字符串以 null 结尾
                    std::cout << "\n[Server Message] " << buffer << std::endl; // 打印接收到的服务器消息，并换行使其更美观
                    std::cout << "[ayp@VM-8-11-centos udp]# ";                 // 重新显示客户端命令提示符
                    fflush(stdout);                                            // 刷新标准输出，确保提示符立即显示
                }
                else if (n == 0)
                {
                    std::cout << "[Client] 服务器可能已关闭连接" << std::endl; // 提示服务器可能关闭
                    break;                                                     // 退出循环，结束线程
                }
                else
                {
                    std::cerr << "[Client] 接收消息失败: " << strerror(errno) << std::endl; // 错误输出
                    break;                                                                  // 接收出错，退出循环
                }
            }
            return nullptr; // 线程返回
        }

        // 运行客户端
        void run()
        {
            pthread_create(&_reader, nullptr, readMessage, (void *)&_socketfd); // 创建读消息线程

            struct sockaddr_in serverAddr;                              // 服务器地址结构
            memset(&serverAddr, 0, sizeof(serverAddr));                 // 初始化为 0
            serverAddr.sin_family = AF_INET;                            // IPv4
            serverAddr.sin_addr.s_addr = inet_addr(_server_ip.c_str()); // 设置服务器 IP 地址
            serverAddr.sin_port = htons(_server_port);                  // 设置服务器端口号

            std::string message; // 存储用户输入的消息
            while (!_quit)       // 循环运行，直到 _quit 为 true
            {
                std::cout << "[ayp@VM-8-11-centos udp]# "; // 客户端命令提示符
                std::getline(std::cin, message);           // 从标准输入读取一行用户输入

                if (strcasecmp(message.c_str(), "quit") == 0)
                {                                                               // 如果输入 "quit" (忽略大小写)
                    std::cout << "[Client] 客户端程序即将退出..." << std::endl; // 提示退出信息
                    _quit = true;                                               // 设置退出标志
                    break;                                                      // 退出发送循环
                }

                // 发送消息到服务器
                ssize_t ret = sendto(_socketfd, message.c_str(), message.size(), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
                if (ret == -1)
                {
                    std::cerr << "[Client] 消息发送失败: " << strerror(errno) << std::endl; // 错误输出
                }
            }

            pthread_cancel(_reader);                               // 取消读取线程, 尝试终止readMessage循环 (注意: 可能需要更优雅的线程退出机制)
            pthread_join(_reader, nullptr);                        // 等待读取线程结束
            close(_socketfd);                                      // 关闭 socket 文件描述符
            std::cout << "[Client] 客户端已安全退出" << std::endl; // 提示客户端退出完成
        }

        // 析构函数
        ~udpClient()
        {
            // 资源清理在 run() 方法的退出部分完成，这里析构函数体为空
        }

    private:
        int _socketfd;          // socket 文件描述符
        std::string _server_ip; // 服务器 IP 地址
        uint16_t _server_port;  // 服务器端口号
        bool _quit;             // 退出标志，用于控制客户端程序循环
        pthread_t _reader;      // 接收消息线程的线程 ID
    };
}