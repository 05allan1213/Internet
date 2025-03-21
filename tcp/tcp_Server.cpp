#include "tcp_Server.hpp"
#include "daemon.hpp"
#include <memory>

using namespace server;

static void Usage(std::string proc)
{
    std::cout << "\nUsage:\n\t" << proc << " local_port\n\n";
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
    daemonSelf();
    tsvr->start();

    return 0;
}