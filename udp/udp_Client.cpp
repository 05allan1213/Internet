#include "udp_Client.hpp"
#include <memory>

using namespace Client;
static void Usage(std::string proc)
{
    std::cout << "\nUsage:\n\t" << proc << " server_ip server_port\n\n";
}

// ./udpClient server_ip server_port
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        Usage(argv[0]);
        exit(USAGE_ERROR);
    }

    std::string server_ip = argv[1];
    uint16_t server_port = atoi(argv[2]);

    std::unique_ptr<udpClient> ucli(new udpClient(server_ip, server_port));

    ucli->initClient();
    ucli->run();

    return 0;
}