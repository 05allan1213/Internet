#include "udp_Client.hpp"
#include <memory>
#include <iostream> // 引入iostream，虽然udp_Client.hpp已经包含了，但为了代码独立性

using namespace Client;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 打印程序使用方法
static void Usage(std::string proc)
{
    std::cout << "\nUsage: " << proc << " server_ip server_port\n"; // 简化Usage输出
    std::cout << "Example: " << proc << " 127.0.0.1 8080\n\n";      // 添加示例
}

// 客户端主函数
int main(int argc, char *argv[])
{
    // 检查命令行参数数量
    if (argc != 3)
    {
        Usage(argv[0]);    // 打印使用方法
        exit(USAGE_ERROR); // 退出程序，返回用法错误码
    }

    std::string server_ip = argv[1];      // 获取服务器IP地址
    uint16_t server_port = atoi(argv[2]); // 获取服务器端口号，并转换为uint16_t

    // 使用 std::make_unique 创建 udpClient 智能指针，自动管理内存
    std::unique_ptr<udpClient> ucli = make_unique<udpClient>(server_ip, server_port);

    ucli->initClient(); // 初始化客户端
    ucli->run();        // 运行客户端

    return 0; // 程序正常退出
}