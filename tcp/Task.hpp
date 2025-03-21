#pragma once

#include <iostream>
#include <cstdio>
#include <string>
#include <functional>
#include <mutex>

std::mutex cerr_mutex;

void serviceIO(int socket)
{
    char buffer[1024];
    while (true)
    {
        ssize_t n = read(socket, buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = 0;
            std::cout << "receive message: " << buffer << std::endl;

            std::string outbuffer = buffer;
            outbuffer += " | server[echo]";

            write(socket, outbuffer.c_str(), outbuffer.size());
        }
        else if (n == 0) // 代表client退出
        {
            logMessage(NORMAL, "client quit, me too");
            break;
        }
    }
    close(socket);
}

class Task
{
    using func_t = std::function<void(int)>;

public:
    Task() = default;
    Task(int socket, func_t func)
        : _socket(socket), _callback(func) {}

    void operator()()
    {
        _callback(_socket);
    }

private:
    int _socket;
    func_t _callback;
};
