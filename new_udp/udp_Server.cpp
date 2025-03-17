#include "udp_Server.hpp"
#include "onlineUser.hpp"
#include <memory>
#include <fstream>
#include <unordered_map>
#include <signal.h>
#include <iostream> // 引入iostream，虽然udp_Server.hpp已经包含了，但为了代码独立性
#include <sstream>  // 引入stringstream，用于更方便的字符串处理

using namespace Server;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

const std::string dictTxt = "./dict.txt";          // 字典文件路径
std::unordered_map<std::string, std::string> dict; // 存储字典的 unordered_map，key为单词，value为解释
OnlineUser onlineUser;                             // 全局在线用户管理对象

// 打印服务器程序使用方法
static void Usage(std::string proc)
{
    std::cout << "\nUsage: " << proc << " local_port\n"; // 简化Usage输出
    std::cout << "Example: " << proc << " 8080\n\n";     // 添加示例
}

// 分割字符串函数，根据分隔符sep将target分割为两部分
static bool cutString(const std::string &target, std::string *s1, std::string *s2, const std::string &sep)
{
    size_t pos = target.find(sep); // 查找分隔符sep在target中首次出现的位置
    if (pos == std::string::npos)  // 如果未找到分隔符
    {
        return false; // 返回false表示分割失败
    }
    *s1 = target.substr(0, pos);           // 提取分隔符前的子字符串
    *s2 = target.substr(pos + sep.size()); // 提取分隔符后的子字符串
    return true;                           // 返回true表示分割成功
}

// 初始化字典，从字典文件加载数据
static void initDict()
{
    std::ifstream in(dictTxt); // 以文本模式打开字典文件
    if (!in.is_open())         // 检查文件是否成功打开
    {
        std::cerr << "[Server] 打开字典文件 " << dictTxt << " 失败" << std::endl; // 错误输出
        exit(OPEN_ERROR);                                                         // 退出程序，返回文件打开错误码
    }
    dict.clear();                  // 清空字典，重新加载
    std::string line;              // 存储从文件中读取的每一行
    std::string key, value;        // 存储分割后的键值对
    while (std::getline(in, line)) // 逐行读取文件内容
    {
        if (cutString(line, &key, &value, ":")) // 使用冒号分割每行，提取键值对
        {
            dict.insert(std::make_pair(key, value)); // 将键值对插入字典
        }
    }
    in.close(); // 关闭文件

    std::cout << "[Server] 字典加载成功，共加载 " << dict.size() << " 个词条" << std::endl; // 打印字典加载成功的消息和词条数量
}

// 信号处理函数，用于重新加载字典
void reload(int signo)
{
    (void)signo;                                                      // 避免编译器警告未使用参数
    std::cout << "[Server] 接收到信号，重新加载字典..." << std::endl; // 打印重新加载字典的消息
    initDict();                                                       // 调用initDict函数重新加载字典
}

// 调试打印字典内容
static void debugPrint()
{
    std::cout << "[Server] 当前字典内容:" << std::endl;
    for (const auto &item : dict) // 使用范围for循环遍历字典
    {
        std::cout << "  " << item.first << " : " << item.second << std::endl; // 打印每个词条的键值对
    }
}

// 处理客户端发来的消息（查字典业务逻辑）
void handlerMessage(int sockfd, const std::string &clientip, const std::string &clientport, const std::string &message)
{
    std::string response_message;   // 存储响应消息
    auto iter = dict.find(message); // 在字典中查找消息（单词）
    if (iter != dict.end())         // 如果找到
    {
        response_message = iter->second; // 获取字典中的解释作为响应消息
    }
    else
    {
        response_message = "未找到该词条"; // 如果未找到，设置响应消息为 "not found"
    }

    // 发送回复消息给客户端
    struct sockaddr_in clientAddr;                            // 客户端地址结构
    bzero(&clientAddr, sizeof(clientAddr));                   // 清零结构体
    clientAddr.sin_family = AF_INET;                          // IPv4
    clientAddr.sin_port = htons(std::stoi(clientport));       // 设置客户端端口号
    clientAddr.sin_addr.s_addr = inet_addr(clientip.c_str()); // 设置客户端IP地址

    ssize_t ret = sendto(sockfd, response_message.c_str(), response_message.size(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (ret == -1)
    {
        std::cerr << "[Server] 回复消息给 " << clientip << ":" << clientport << " 失败: " << strerror(errno) << std::endl; // 错误输出
    }
}

// 执行客户端发来的命令 (存在安全风险，示例仅供演示)
void execCommand(int sockfd, const std::string &clientip, const std::string &clientport, const std::string &cmd)
{
    // 安全检查：禁止执行危险命令，例如 rm, mv, rmdir 等
    if (cmd.find("rm") != std::string::npos || cmd.find("mv") != std::string::npos || cmd.find("rmdir") != std::string::npos)
    {
        std::cerr << "[Server] 来自 " << clientip << ":" << clientport << " 的非法操作尝试: " << cmd << " 已被阻止" << std::endl; // 警告信息
        std::string warningMsg = "危险命令已被阻止";
        struct sockaddr_in clientAddr;
        bzero(&clientAddr, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(std::stoi(clientport));
        clientAddr.sin_addr.s_addr = inet_addr(clientip.c_str());
        sendto(sockfd, warningMsg.c_str(), warningMsg.size(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        return; // 拒绝执行危险命令
    }

    std::string response;               // 存储命令执行结果
    FILE *fp = popen(cmd.c_str(), "r"); // 执行 shell 命令
    if (fp == nullptr)                  // 检查 popen 是否成功
    {
        response = cmd + " 执行失败"; // 如果失败，设置错误消息
        std::cerr << "[Server] 命令 " << cmd << " 执行失败" << std::endl;
    }
    else
    {
        char line[1024];                      // 缓冲区，用于读取命令输出
        while (fgets(line, sizeof(line), fp)) // 逐行读取命令输出
        {
            response += line; // 追加到响应字符串
        }
        pclose(fp); // 关闭文件指针
    }

    // 发送命令执行结果给客户端
    struct sockaddr_in clientAddr;
    bzero(&clientAddr, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(std::stoi(clientport));
    clientAddr.sin_addr.s_addr = inet_addr(clientip.c_str());

    sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
}

// 路由消息，处理 "online", "offline" 命令和广播消息
void routeMessage(int sockfd, const std::string &clientip, const std::string &clientport, const std::string &message)
{
    uint16_t port = static_cast<uint16_t>(std::stoi(clientport)); // 将端口号字符串转换为 uint16_t

    if (message == "online") // 如果收到 "online" 命令
    {
        onlineUser.addUser(clientip, port); // 添加用户到在线列表
    }
    else if (message == "offline") // 如果收到 "offline" 命令
    {
        onlineUser.delUser(clientip, port); // 从在线列表删除用户
    }
    else if (message == "who")
    { // 如果收到 "who" 命令, 尚未实现，可以扩展显示在线用户列表
        std::string response = "当前在线用户功能尚未实现";
        struct sockaddr_in clientAddr;
        bzero(&clientAddr, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(std::stoi(clientport));
        clientAddr.sin_addr.s_addr = inet_addr(clientip.c_str());
        sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    }
    else if (onlineUser.isOnline(clientip, port)) // 如果用户已在线
    {
        onlineUser.broadcastMessage(sockfd, clientip, port, message); // 广播消息
    }
    else // 用户不在线
    {
        struct sockaddr_in clientAddr;
        bzero(&clientAddr, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(std::stoi(clientport));
        clientAddr.sin_addr.s_addr = inet_addr(clientip.c_str());

        std::string response = "你当前处于离线状态，请先发送 'online' 上线"; // 提示用户先上线
        sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    }
}

// 服务器主函数
int main(int argc, char *argv[])
{
    // 检查命令行参数数量
    if (argc != 2)
    {
        Usage(argv[0]);    // 打印使用方法
        exit(USAGE_ERROR); // 退出程序，返回用法错误码
    }
    uint16_t port = atoi(argv[1]); // 获取并转换端口号

    signal(2, reload); // 注册 SIGHUP 信号处理函数，用于重新加载字典
    initDict();        // 初始化字典
    // debugPrint(); // 可选：启动时打印字典内容

    // std::unique_ptr<udpServer> usvr = make_unique<udpServer>(routeMessage, port);
    // std::unique_ptr<udpServer> usvr = make_unique<udpServer>(handlerMessage, port);
    std::unique_ptr<udpServer> usvr = make_unique<udpServer>(execCommand, port);

    usvr->initServer(); // 初始化服务器
    usvr->start();      // 启动服务器

    return 0; // 程序正常退出
}