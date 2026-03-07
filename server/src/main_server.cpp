#include "common/include/common.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

#define PATH "/home/latarnik3/tftpserver/data"

uint16_t extractOpcode(const char* buffer){
    return (static_cast<u_int16_t>(buffer[0]) << 8) | static_cast<u_int16_t>(buffer[1]);
}

uint16_t extractBlockNumber(const char* buffer) {
    return (static_cast<uint8_t>(buffer[2]) << 8) | static_cast<uint8_t>(buffer[3]);
}

bool waitForAck(int w_socket, sockaddr_in &client_addr, socklen_t &client_addr_len, uint16_t expected_block){
    char ack_buffer[8];
    
    int bytes_received = recvfrom(w_socket, ack_buffer, 8, 0, (sockaddr *)&client_addr, &client_addr_len);
    
    if(bytes_received >= 4){
        uint16_t ack_opcode = extractOpcode(ack_buffer);
        uint16_t ack_block_num = extractBlockNumber(ack_buffer);
    
        if(ack_opcode == 4 && ack_block_num == expected_block) return true;
        else return false;
    }
    return false; 
}

void sendData(std::ifstream &file, char (&tx_buffer)[TX_BUFFER_SIZE], int w_socket, sockaddr_in &client_addr){
    uint16_t opcode = 3;
    uint16_t block_number = 1;

    *(uint16_t*)(&tx_buffer[0]) = htons(opcode);

    socklen_t client_addr_len = sizeof(client_addr);
    
    while(true){
        *(uint16_t*)(&tx_buffer[2]) = htons(block_number);
        
        file.read((&tx_buffer[4]), TX_BUFFER_SIZE);
        int bytes_read = file.gcount();
        
        sendto(w_socket, tx_buffer, 4+bytes_read, 0, (const sockaddr *)&client_addr, sizeof(client_addr));

        if(waitForAck(w_socket, client_addr, client_addr_len, block_number)){
            block_number++; 
        }
        else{
            std::cout<<"Transmission error!"<<std::endl;
        }
        if(bytes_read < 512) break;
    }
}



int main(){
    int w_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if(w_socket <0){
        std::cerr<<"Socket error"<<std::endl;
        return 0;
    }
    std::cout<<"Socket created"<<std::endl;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(w_socket, (const sockaddr *)&server_addr, sizeof(server_addr))<0){
        std::cerr<<"Bind error"<<std::endl;
        return 1;
    }
    std::cout<<"Bind executed"<<std::endl;

    char rx_buffer[RX_BUFFER_SIZE];
    memset(&client_addr, 0, sizeof(client_addr));
    if(recvfrom(w_socket, rx_buffer, RX_BUFFER_SIZE, 0, (sockaddr *)&client_addr, &client_addr_len)<0){
        std::cerr<<"Recvfrom error"<<std::endl;
        return 1;
    }

    char ip_string[ADDRESS_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_string, sizeof(ip_string));
    std::cout<<"Connected client: "<<ip_string<<":"<<ntohs(client_addr.sin_port)<<std::endl;

    char tx_buffer[TX_BUFFER_SIZE];
    uint16_t op = extractOpcode(rx_buffer);
    if(op == 1){
        std::cout<<"RRQ"<<std::endl;
        std::string file_name(&rx_buffer[2]);
        std::filesystem::path file_path = std::filesystem::path(PATH) / file_name; 
        std::ifstream file(file_path, std::ios::binary);

        if(file){
            std::cout<<"[FILE FOUND]"<<std::endl;
            uintmax_t f_size = std::filesystem::file_size(file_path);
            sendData(file, tx_buffer, w_socket, client_addr);
        }
        else{
            std::cout<<"[FILE NOT FOUND]"<<std::endl;
        }

    }else if(op == 2){
        std::cout<<"WRQ"<<std::endl;
    }

    return 0;
}