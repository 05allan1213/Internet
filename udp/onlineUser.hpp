#pragma once

#include <iostream>
#include <string>
#include <strings.h>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class User
{
public:
    User(const std::string &ip, const uint16_t &port)
        : _ip(ip), _port(port)
    {
    }

    std::string ip()
    {
        return _ip;
    }

    uint16_t port()
    {
        return _port;
    }

    ~User()
    {
    }

private:
    std::string _ip;
    uint16_t _port;
};

class OnlineUser
{
public:
    OnlineUser()
    {
    }
    void addUser(const std::string &ip, const uint16_t &port)
    {
        std::string id = ip + "-" + std::to_string(port);
        users.insert(std::make_pair(id, User(ip, port)));
    }
    void delUser(const std::string &ip, const uint16_t &port)
    {
        std::string id = ip + "-" + std::to_string(port);
        users.erase(id);
    }
    bool isOnline(const std::string &ip, const uint16_t &port)
    {
        std::string id = ip + "-" + std::to_string(port);
        return users.find(id) == users.end() ? false : true;
    }
    void broadcastMessage(int socketfd, const std::string &ip, const uint16_t &port, const std::string &message)
    {
        for (auto &user : users)
        {
            struct sockaddr_in client;
            bzero(&client, sizeof(client));

            client.sin_family = AF_INET;
            client.sin_port = htons(user.second.port());
            client.sin_addr.s_addr = inet_addr(user.second.ip().c_str());
            std::string s = ip + "-" + std::to_string(port) + "# ";
            s += message;
            sendto(socketfd, s.c_str(), s.size(), 0, (struct sockaddr *)&client, sizeof(client));
        }
    }
    ~OnlineUser()
    {
    }

private:
    std::unordered_map<std::string, User> users;
};