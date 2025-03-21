#include "cal_Client.hpp"
#include <memory>

using namespace client;

static void Usage(std::string proc)
{
    std::cout << "\nUsage:\n\t" << proc << " serverip serverport\n\n";
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        Usage(argv[0]);
        exit(USAGE_ERROR);
    }
    std::string serverip = argv[1];
    uint16_t serverport = atoi(argv[2]);

    std::unique_ptr<TcpClient> tcli(new TcpClient(serverip, serverport));
    tcli->initClient();
    tcli->start();

    return 0;
}