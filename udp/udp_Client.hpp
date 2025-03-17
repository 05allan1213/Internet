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

namespace Client
{
    enum
    {
        USAGE_ERROR = 1,
        SOCKET_ERROR,
        BIND_ERROR
    };
    class udpClient
    {
    public:
        udpClient(const std::string &ip, const uint16_t &port)
            : _server_ip(ip), _server_port(port), _socketfd(-1), _quit(false)
        {
        }
        void initClient()
        {
            // 创建socket
            _socketfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (_socketfd == -1)
            {
                std::cerr << "socket error: " << strerror(errno) << std::endl;
                exit(SOCKET_ERROR);
            }
            std::cout << "socket success: " << _socketfd << std::endl;
            // 2. client必须要bind,但不需要程序员显示bind
            // 写服务器的是一家公司，写客户端是无数家公司 -- 由OS自动bind
        }

        static void *readMessage(void *args)
        {
            int sockfd = *(static_cast<int *>((args)));
            pthread_detach(pthread_self());

            while (true)
            {
                char buffer[1024];
                struct sockaddr_in temp;
                socklen_t temp_len = sizeof(temp);
                size_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&temp, &temp_len);
                if (n > 0)
                {
                    buffer[n] = 0;
                }
                std::cout << "\n"
                          << buffer << std::endl;
            }
            return nullptr;
        }
        void run()
        {
            pthread_create(&_reader, nullptr, readMessage, (void *)&_socketfd);

            struct sockaddr_in server;
            memset(&server, 0, sizeof(server));
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(_server_ip.c_str());
            server.sin_port = htons(_server_port);

            std::string message;
            while (!_quit)
            {
                // std::cout << "[ayp@VM-8-11-centos udp]# ";
                std::getline(std::cin, message);
                sendto(_socketfd, message.c_str(), message.size(), 0, (struct sockaddr *)&server, sizeof(server));
            }
        }
        ~udpClient()
        {
        }

    private:
        int _socketfd;
        std::string _server_ip;
        uint16_t _server_port;
        bool _quit;
        pthread_t _reader;
    };
}