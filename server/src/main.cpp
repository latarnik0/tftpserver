#include "server.hpp"
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

int createSocket()
{
    int main_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (main_socket < 0) 
    {
        std::cerr << "Socket creation error" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Socket created" << std::endl;
    return main_socket;
}

void bindSocket(int main_sock, const sockaddr_in& server_address)
{
    if (bind(main_sock, (const sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        std::cerr << "Bind error" << std::endl;
        close(main_sock);
        exit(EXIT_FAILURE); 
    }
    std::cout << "Bind executed" << std::endl;
}

sockaddr_in makeServerAddress()
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MAIN_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return server_addr;
}

bool waitForInitialPacket(int main_socket, char* rx_buffer, sockaddr_in& client_addr, socklen_t& client_addr_len)
{
    struct timeval tv_infinite;
    tv_infinite.tv_sec = 0;  
    tv_infinite.tv_usec = 0; 
    
    setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, &tv_infinite, sizeof(tv_infinite));

    if (recvfrom(main_socket, rx_buffer, RX_BUFFER_SIZE, 0, (sockaddr *)&client_addr, &client_addr_len) < 0) 
    {
        std::cerr << "Recvfrom error" << std::endl;
        return false;
    }
    return true;
}

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