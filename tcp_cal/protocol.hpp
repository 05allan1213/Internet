#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

#define SEP " "
#define SEP_LEN strlen(SEP) // 不要使用sizeof()
#define LINE_SEP "\r\n"
#define LINE_SEP_LEN strlen(LINE_SEP)

enum
{
    OK = 0,
    DIV_ZERO = 1,
    MOD_ZERO = 2,
    OP_UNKNOWN = 3,
};

// "x op y" -> "content_len"\r\n"x op y"\r\n
// "exitcode result" -> "content_len"\r\n"exitcode result"\r\n
std::string enLength(const std::string &text)
{
    std::string send_string = std::to_string(text.size());
    send_string += LINE_SEP;
    send_string += text;
    send_string += LINE_SEP;

    return send_string;
}

// "content_len"\r\n"exitcode result"\r\n
bool deLength(const std::string &package, std::string *text)
{
    auto pos = package.find(LINE_SEP);
    if (pos == std::string::npos)
    {
        return false;
    }
    std::string text_len_string = package.substr(0, pos);
    int text_len = std::stoi(text_len_string);
    std::string text_content = package.substr(pos + LINE_SEP_LEN, text_len);

    return true;
}

class Request
{
public:
    Request()
    {
    }
    Request(int x_, int y_, char op_)
        : x(x_), y(y_), op(op_)
    {
    }

    bool serialize(std::string *out)
    {
        *out = "";
        // 结构化 -> "x op y\r\n"
        std::string x_string = std::to_string(x);
        std::string y_string = std::to_string(y);

        *out += x_string;
        *out += SEP;
        *out += op;
        *out += SEP;
        *out += y_string;

        return true;
    }

    bool deserialize(const std::string &in)
    {
        // "x op y" -> 结构化
        auto left = in.find(SEP);
        auto right = in.rfind(SEP);
        if (left == std::string::npos || right == std::string::npos || left == right)
        {
            return false;
        }

        if (right - (left + SEP_LEN) != 1)
        {
            return false;
        }

        std::string x_string = in.substr(0, left);
        std::string y_string = in.substr(right + SEP_LEN);

        if (x_string.empty())
        {
            return false;
        }

        if (y_string.empty())
        {
            return false;
        }

        x = std::stoi(x_string);
        y = std::stoi(y_string);
        op = in[left + SEP_LEN];

        return true;
    }

public:
    // "x op y"
    int x;
    int y;
    char op;
};

class Response
{
public:
    Response()
        : exitcode(0), result(0)
    {
    }

    Response(int exitcode_, int result_)
        : exitcode(exitcode_), result(result_)
    {
    }

    bool serialize(std::string *out)
    {
        *out = "";
        std::string exitcode_string = std::to_string(exitcode);
        std::string result_string = std::to_string(result);

        *out += exitcode_string;
        *out += SEP;
        *out += result_string;

        return true;
    }

    bool deserialize(const std::string &in)
    {
        // "exitcode result" -> 结构化
        auto mid = in.find(SEP);
        if (mid == std::string::npos)
        {
            return false;
        }
        std::string exitcode_string = in.substr(0, mid);
        std::string result_string = in.substr(mid + SEP_LEN);

        if (exitcode_string.empty() || result_string.empty())
        {
            return false;
        }
        exitcode = std::stoi(exitcode_string);
        result = std::stoi(result_string);

        return true;
    }

public:
    int exitcode; // 0: OK, !0: ERROR
    int result;   // 计算结果
};

// "content_len"\r\n"x op y"\r\n
bool recvPackage(int socketfd, std::string &inbuffer, std::string *text)
{
    char buffer[1024];
    while (true)
    {
        ssize_t n = recv(socketfd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0)
        {
            buffer[n] = 0;
            inbuffer += buffer;
            // 分析处理
            auto pos = inbuffer.find(LINE_SEP);
            if (pos == std::string::npos)
            {
                continue;
            }
            std::string text_len_string = inbuffer.substr(0, pos);
            int text_len = std::stoi(text_len_string);
            int total_len = text_len_string.size() + 2 * LINE_SEP_LEN + text_len;
            // text_len_string + "\r\n" + text + "\r\n" <= inbuffer.size()
            if (inbuffer.size() < total_len)
            {
                continue;
            }
            std::cout << "处理前#inbuffer: \n"
                      << inbuffer << std::endl;
            // 至少有一个完整的报文
            *text = inbuffer.substr(pos + LINE_SEP_LEN);
            inbuffer = inbuffer.substr(total_len);
            std::cout << "处理后#inbuffer: \n"
                      << inbuffer << std::endl;
            break;
        }
        else
        {
            return false;
        }
    }
    return true;
}

// bool recvRequestAll(int socketfd, std::vector<std::string> *out)
// {
//     std::string line;
//     while(recvRequest(socketfd, &line))
//     {
//         out->push_back(line);
//     }
// }