#pragma once

#include <iostream>
#include <string>
#include <strings.h> // for bzero
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class User
{
public:
    // 构造函数，初始化用户的IP地址和端口号
    User(const std::string &ip, const uint16_t &port)
        : _ip(ip), _port(port)
    {
        // 构造函数体为空
    }

    // 获取用户IP地址
    std::string ip() const // 添加 const 修饰符，表示该方法不会修改对象状态
    {
        return _ip;
    }

    // 获取用户端口号
    uint16_t port() const // 添加 const 修饰符，表示该方法不会修改对象状态
    {
        return _port;
    }

    // 析构函数
    ~User()
    {
        // 析构函数体为空，无需手动释放资源
    }

private:
    std::string _ip; // 用户IP地址
    uint16_t _port;  // 用户端口号
};

class OnlineUser
{
public:
    OnlineUser()
    {
        // 构造函数体为空
    }

    // 添加用户到在线用户列表
    void addUser(const std::string &ip, const uint16_t &port)
    {
        std::string id = generateUserId(ip, port);                                        // 生成用户ID
        users.insert(std::make_pair(id, User(ip, port)));                                 // 将用户ID和User对象添加到unordered_map中
        std::cout << "[OnlineUser] 用户 " << ip << ":" << port << " 上线了" << std::endl; // 打印用户上线信息
    }

    // 从在线用户列表删除用户
    void delUser(const std::string &ip, const uint16_t &port)
    {
        std::string id = generateUserId(ip, port);                                        // 生成用户ID
        users.erase(id);                                                                  // 从unordered_map中移除用户
        std::cout << "[OnlineUser] 用户 " << ip << ":" << port << " 下线了" << std::endl; // 打印用户下线信息
    }

    // 检查用户是否在线
    bool isOnline(const std::string &ip, const uint16_t &port) const // 添加 const 修饰符
    {
        std::string id = generateUserId(ip, port); // 生成用户ID
        return users.find(id) != users.end();      // 在unordered_map中查找用户ID，如果找到则返回true，否则返回false
    }

    // 广播消息给所有在线用户，除了发送者自己
    void broadcastMessage(int socketfd, const std::string &senderIp, const uint16_t &senderPort, const std::string &message)
    {
        std::string senderId = generateUserId(senderIp, senderPort);                                        // 生成发送者ID
        std::string formattedMessage = "[" + senderIp + ":" + std::to_string(senderPort) + "]# " + message; // 格式化消息，包含发送者信息

        for (const auto &pair : users) // 使用范围for循环遍历unordered_map
        {
            const User &user = pair.second;
            if (generateUserId(user.ip(), user.port()) != senderId) // 不要发给自己
            {
                struct sockaddr_in clientAddr;
                bzero(&clientAddr, sizeof(clientAddr)); // 清零结构体
                clientAddr.sin_family = AF_INET;
                clientAddr.sin_port = htons(user.port());
                clientAddr.sin_addr.s_addr = inet_addr(user.ip().c_str());

                ssize_t ret = sendto(socketfd, formattedMessage.c_str(), formattedMessage.size(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                if (ret == -1)
                {
                    std::cerr << "[OnlineUser] 广播消息给 " << user.ip() << ":" << user.port() << " 失败: " << strerror(errno) << std::endl; // 错误处理
                }
                else
                {
                    std::cout << "[OnlineUser] 广播消息给 " << user.ip() << ":" << user.port() << " 成功" << std::endl; // 打印广播成功信息
                }
            }
        }
    }

    // 析构函数
    ~OnlineUser()
    {
        // 析构函数体为空
    }

private:
    std::unordered_map<std::string, User> users; // 使用 unordered_map 存储在线用户，key为用户ID，value为User对象

    // 生成用户唯一ID，使用IP地址和端口号组合
    std::string generateUserId(const std::string &ip, uint16_t port) const // 添加 const 修饰符
    {
        return ip + "-" + std::to_string(port);
    }
};