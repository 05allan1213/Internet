#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "error.hpp"

namespace client
{
    class TcpClient
    {
    public:
        TcpClient(const std::string &serverip, const uint16_t &serverport)
            : _socketfd(-1), _serverip(serverip), _serverport(serverport)
        {
        }

        void initClient()
        {
            // 1. 创建套接字
            _socketfd = socket(AF_INET, SOCK_STREAM, 0);
            if (_socketfd < 0)
            {
                std::cerr << "socket create error" << std::endl;
                exit(SOCKET_ERROR);
            }
            // 2. TCP的客户端要bind，但不要显示地bind，让OS自动完成bind操作(尤其是client port)
            // 3. 要不要listen？ 不要
            // 4. 要不要accept？ 不要
        }

        void start()
        {
            // 5. 发起链接
            struct sockaddr_in serveraddr;
            memset(&serveraddr, 0, sizeof(serveraddr));
            serveraddr.sin_family = AF_INET;
            serveraddr.sin_port = htons(_serverport);
            serveraddr.sin_addr.s_addr = inet_addr(_serverip.c_str());

            if (connect(_socketfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0)
            {
                std::cerr << "connect error" << std::endl;
                exit(CONNECT_ERROR);
            }
            else
            {
                std::string msg;
                while (true)
                {
                    std::cout << "请输入发送内容# ";
                    std::getline(std::cin, msg);
                    write(_socketfd, msg.c_str(), msg.size());

                    char buffer[1024];
                    int n = read(_socketfd, buffer, sizeof(buffer) - 1);
                    if (n > 0)
                    {
                        buffer[n] = 0;
                        std::cout << "server echo# " << buffer << std::endl;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        ~TcpClient()
        {
            if (_socketfd >= 0)
            {
                close(_socketfd);
            }
        }

    private:
        int _socketfd;
        std::string _serverip;
        uint16_t _serverport;
    };
}