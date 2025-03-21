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
#include "protocol.hpp"

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
                std::string line;
                std::string inbuffer;
                while (true)
                {
                    std::cout << "请输入>>> ";
                    std::getline(std::cin, line);

                    Request req = ParseLine(line);
                    std::string content;
                    req.serialize(&content);
                    std::string send_string = enLength(content);
                    send(_socketfd, send_string.c_str(), send_string.size(), 0);

                    std::string package, text;
                    // "content_len"\r\n"exitcode result"\r\n
                    if (!recvPackage(_socketfd, inbuffer, &package))
                    {
                        continue;
                    }
                    if (!deLength(package, &text))
                    {
                        continue;
                    }
                    // "exitcode result"
                    Response resp;
                    resp.deserialize(text);
                    std::cout << "exitcode: " << resp.exitcode << std::endl;
                    std::cout << "result: " << resp.result << std::endl;
                }
            }
        }

        Request ParseLine(const std::string &line)
        {
            int status = 0; // 0: 操作符之前，1：碰到操作符，2：操作数之后
            int i = 0;
            int cnt = line.size();
            std::string left, right;
            char op;
            while (i < cnt)
            {
                switch (status)
                {
                case 0:
                {
                    if (!isdigit(line[i]))
                    {
                        op = line[i];
                        status = 1;
                    }
                    else
                    {
                        left.push_back(line[i++]);
                    }
                }
                break;
                case 1:
                    i++;
                    status = 2;
                    break;
                case 2:
                    right.push_back(line[i++]);
                    break;
                }
            }
            return Request(std::stoi(left), std::stoi(right), op);
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