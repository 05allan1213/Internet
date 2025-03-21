#include "cal_Server.hpp"
#include <memory>

using namespace server;

static void Usage(std::string proc)
{
    std::cout << "\nUsage:\n\t" << proc << " local_port\n\n";
}

// req:里面一定是我们处理好的一个完整的请求对象
// resp:根据req，进行业务逻辑，填充resp，不用管理任何读取和写入，序列化和反序列化等任何细节
bool cal(const Request &req, Response &resp)
{
    resp.exitcode = OK;
    resp.result = OK;

    switch (req.op)
    {
    case '+':
        resp.result = req.x + req.y;
        break;
    case '-':
        resp.result = req.x - req.y;
        break;
    case '*':
        resp.result = req.x * req.y;
        break;
    case '/':
        if (req.y == 0)
        {
            resp.exitcode = DIV_ZERO;
        }
        else
        {
            resp.result = req.x / req.y;
        }
        break;
    case '%':
        if (req.y == 0)
        {
            resp.exitcode = MOD_ZERO;
        }
        else
        {
            resp.result = req.x % req.y;
        }
        break;
    default:
        resp.exitcode = OP_UNKNOWN;
        break;
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(USAGE_ERROR);
    }
    uint16_t port = atoi(argv[1]);

    std::unique_ptr<TcpServer> tsvr(new TcpServer(port));
    tsvr->initServer();
    tsvr->start(cal);

    return 0;
}