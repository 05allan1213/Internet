#include "udp_Server.hpp"
#include "onlineUser.hpp"
#include <memory>
#include <fstream>
#include <unordered_map>
#include <signal.h>

using namespace Server;

const std::string dictTxt = "./dict.txt";
std::unordered_map<std::string, std::string> dict;

static void Usage(std::string proc) // 输出服务端的使用说明
{
    std::cout << "\nUsage:\n\t" << proc << " local_port\n\n"; // 提示用户需要提供本地IP和端口号
}

static bool cutString(const std::string &target, std::string *s1, std::string *s2, std::string sep)
{
    auto pos = target.find(sep);
    if (pos == std::string::npos)
    {
        return false;
    }
    *s1 = target.substr(0, pos);
    *s2 = target.substr(pos + sep.size());
    return true;
}

static void initDict()
{
    std::ifstream in(dictTxt, std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "open file " << dictTxt << " error" << std::endl;
        exit(OPEN_ERROR);
    }
    std::string line;
    std::string key, value;
    while (std::getline(in, line))
    {
        if (cutString(line, &key, &value, ":"))
        {
            dict.insert(std::make_pair(key, value));
        }
    }

    in.close();

    std::cout << "load dict success" << std::endl;
}
void reload(int signo)
{
    (void)signo;
    initDict();
}
static void debugPrint()
{
    for (auto &item : dict)
    {
        std::cout << item.first << " # " << item.second << std::endl;
    }
}

void handlerMessage(int sockfd, std::string clientip, std::string clientport, std::string message)
{
    // 业务逻辑
    std::string response_message;
    auto iter = dict.find(message);
    if (iter != dict.end())
    {
        response_message = iter->second;
    }
    else
    {
        response_message = "not found";
    }
    // 发送回复消息
    struct sockaddr_in client;
    bzero(&client, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(atoi(clientport.c_str()));
    client.sin_addr.s_addr = inet_addr(clientip.c_str());

    sendto(sockfd, response_message.c_str(), response_message.size(), 0, (struct sockaddr *)&client, sizeof(client));
}

void execCommand(int sockfd, std::string clientip, std::string clientport, std::string cmd)
{
    // 1. cmd解析
    // 2. 如果必要，可能需要创建子进程

    if (cmd.find("rm") != std::string::npos || cmd.find("mv") != std::string::npos || cmd.find("rmdir") != std::string::npos)
    {
        std::cerr << clientip << "[" << clientport << "]# " << "正在做一个非法操作: " << cmd << std::endl;
        return;
    }

    std::string response;
    FILE *fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
    {
        response = cmd + "exec failed";
    }
    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        response += line;
    }

    struct sockaddr_in client;
    bzero(&client, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(atoi(clientport.c_str()));
    client.sin_addr.s_addr = inet_addr(clientip.c_str());

    sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&client, sizeof(client));

    pclose(fp);
}

OnlineUser onlineUser;
void routeMessage(int sockfd, std::string clientip, std::string clientport, std::string message)
{
    uint16_t port = static_cast<uint16_t>(atoi(clientport.c_str()));

    if (message == "online")
    {
        uint16_t port = static_cast<uint16_t>(atoi(clientport.c_str()));
        onlineUser.addUser(clientip, port);
    }
    if (message == "offline")
    {
        uint16_t port = static_cast<uint16_t>(atoi(clientport.c_str()));
        onlineUser.delUser(clientip, port);
    }
    if (onlineUser.isOnline(clientip, atoi(clientport.c_str())))
    {
        onlineUser.broadcastMessage(sockfd, clientip, port, message);
    }
    else
    {
        struct sockaddr_in client;
        bzero(&client, sizeof(client));
        client.sin_family = AF_INET;
        client.sin_port = htons(atoi(clientport.c_str()));
        client.sin_addr.s_addr = inet_addr(clientip.c_str());

        std::string response = "你目前离线,请先上线,运行: online";
        sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&client, sizeof(client));
    }
}
// 启动服务器 —— ./udpServer port
int main(int argc, char *argv[])
{
    // 检查命令行参数数量是否为2（程序名 + 端口号）
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(USAGE_ERROR);
    }
    uint16_t port = atoi(argv[1]); // 将端口号字符串转换为整数
    // std::string ip = argv[1];      // 获取本地IP地址

    // signal(2, reload);
    // initDict();
    // debugPrint();

    // std::unique_ptr<udpServer> usvr(new udpServer(handlerMessage, port));
    // std::unique_ptr<udpServer> usvr(new udpServer(execCommand, port));
    std::unique_ptr<udpServer> usvr(new udpServer(routeMessage, port));

    usvr->initServer();
    usvr->start();

    return 0;
}