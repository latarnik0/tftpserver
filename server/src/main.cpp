#include "server.hpp"
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

int main() 
{
    srand(time(NULL));
    int main_socket = createSocket();
    struct sockaddr_in server_addr = makeServerAddress();
    bindSocket(main_socket, server_addr);

    while (true)
    {
        char rx_buffer[RX_BUFFER_SIZE];
        char tx_buffer[TX_BUFFER_SIZE];
        
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));

        if (!waitForInitialPacket(main_socket, rx_buffer, client_addr, client_addr_len)) {
            close(main_socket);
            continue; 
        }

        char ip_string[ADDRESS_SIZE];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_string, sizeof(ip_string)); 
        
        std::cout << "Connected client: " << ip_string << ":" << ntohs(client_addr.sin_port) << std::endl;
        
        uint16_t op = extractOpcode(rx_buffer);
        
        if(op == 1)
        {
            std::cout << "RRQ" << std::endl;
            sendData(tx_buffer, rx_buffer, client_addr);
        }
        else if(op == 2)
        {
            std::cout << "WRQ" << std::endl;
            waitForData(rx_buffer, tx_buffer, client_addr, client_addr_len);
        }
        else
        {
            std::cout << "Illegal TFTP operation" << std::endl;
            sendError(main_socket, client_addr, 4, "[Illegal TFTP operation]");
            close(main_socket);
            break;
        }
    }
    return 0;
}