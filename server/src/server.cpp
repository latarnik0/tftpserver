#include "server.hpp"
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

uint16_t extractOpcode(const char* buffer){
    return (static_cast<u_int16_t>(buffer[0]) << 8) | static_cast<u_int16_t>(buffer[1]);
}

uint16_t extractBlockNumber(const char* buffer) {
    return (static_cast<uint8_t>(buffer[2]) << 8) | static_cast<uint8_t>(buffer[3]);
}

uint16_t extractErrorId(const char* buffer) {
    return (static_cast<uint8_t>(buffer[2]) << 8) | static_cast<uint8_t>(buffer[3]);
}

void newPort(int &new_socket){
    new_socket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in new_addr;
    memset(&new_addr, 0, sizeof(new_addr));
    new_addr.sin_family = AF_INET;
    new_addr.sin_port = htons(0);
    new_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(new_socket, (const sockaddr *)&new_addr, sizeof(new_addr)) < 0) {
        std::cerr << "Transfer bind error" << std::endl;
    }
}

bool waitForAck(int socket, sockaddr_in &sender_addr, socklen_t &sender_addr_len, uint16_t expected_block){
    char ack_buffer[8];
        
    int bytes_received = recvfrom(socket, ack_buffer, 8, 0, (sockaddr *)&sender_addr, &sender_addr_len);

    if(bytes_received >= 4){
        uint16_t ack_opcode = extractOpcode(ack_buffer);
        uint16_t ack_block_num = extractBlockNumber(ack_buffer);
        
        if(ack_opcode == 4 && ack_block_num == expected_block){
            return true;
        }
        else if(ack_opcode == 5){
            uint16_t error_id = extractErrorId(ack_buffer);
            std::cout<<"[CLIENT ERROR] Transfer aborted. Error ID: "<<error_id<<std::endl;
            return false;
        } 
    }
    else{
        return false;
    }
    return false;
}

void sendError(int socket, sockaddr_in &target_addr, uint16_t error_code, const std::string& error_msg) {
    char error_buffer[516];
    *(uint16_t*)(&error_buffer[0]) = htons(5);
    *(uint16_t*)(&error_buffer[2]) = htons(error_code);
    
    std::strcpy(&error_buffer[4], error_msg.c_str());

    int packet_size = 4 + error_msg.length() + 1;
    
    sendto(socket, error_buffer, packet_size, 0, (const sockaddr *)&target_addr, sizeof(target_addr));
}

void waitForData(char (&rx_buffer)[RX_BUFFER_SIZE], char (&tx_buffer)[TX_BUFFER_SIZE], sockaddr_in &client_addr, socklen_t &client_addr_len){
    int receive_socket;
    newPort(receive_socket);

    std::string file_name(&rx_buffer[2]);
    std::filesystem::path file_path = std::filesystem::path(PATH) / file_name; 
    std::ofstream file(file_path, std::ios::binary);

    uint16_t ack_opcode = 4;
    uint16_t ack_block_number = 0;
    uint16_t recv_opcode;
    uint16_t recv_block_number;

    *(uint16_t*)(&tx_buffer[0]) = htons(ack_opcode);
    *(uint16_t*)(&tx_buffer[2]) = htons(ack_block_number);

    sendto(receive_socket, tx_buffer, 4, 0, (const sockaddr *)&client_addr, sizeof(client_addr));

    while(true){
        struct sockaddr_in sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);
        memset(&sender_addr, 0, sizeof(sender_addr));

        int wait_limit = 4;
        
        int bytes_recv = recvfrom(receive_socket, rx_buffer, RX_BUFFER_SIZE, 0, (sockaddr *)&sender_addr, (socklen_t*)&sender_addr_len);

        if(bytes_recv < 0){
            wait_limit--;
            if(wait_limit == 0){
                break;
            }
            sendto(receive_socket, tx_buffer, 4, 0, (const sockaddr *)&client_addr, sizeof(client_addr));
            continue;

        }
        wait_limit = 4;

        if(sender_addr.sin_port != client_addr.sin_port || sender_addr.sin_addr.s_addr != client_addr.sin_addr.s_addr){
            sendError(receive_socket, sender_addr, 5, "[Unauthorized client]");
            continue;
        }
        
        recv_opcode = extractOpcode(rx_buffer);
        recv_block_number = extractBlockNumber(rx_buffer);

        if(recv_opcode == 5){
            sendError(receive_socket, client_addr, 5, "[Transfer terminated]");
        }
        else if(recv_opcode != 3 && recv_opcode != 5){
            sendError(receive_socket, client_addr, 4, "[Illegal TFTP operation]");
        }

        file.write(&rx_buffer[4], bytes_recv - 4);

        *(uint16_t*)(&tx_buffer[2]) = htons(recv_block_number);
        sendto(receive_socket, tx_buffer, 4, 0, (const sockaddr *)&client_addr, sizeof(client_addr));

        if(bytes_recv < 516) break; 
    }
}

void sendData(char (&tx_buffer)[TX_BUFFER_SIZE], char (&rx_buffer)[RX_BUFFER_SIZE], sockaddr_in &client_addr){
    int transfer_socket;
    newPort(transfer_socket);

    std::string file_name(&rx_buffer[2]);
    std::filesystem::path file_path = std::filesystem::path(PATH) / file_name; 
    std::ifstream file(file_path, std::ios::binary);

    if(!file){
        sendError(transfer_socket, client_addr, 1, "[File not found]");
    }

    uint16_t opcode = 3;
    uint16_t block_number = 1;

    *(uint16_t*)(&tx_buffer[0]) = htons(opcode);

    socklen_t client_addr_len = sizeof(client_addr);

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(transfer_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while(true){
        struct sockaddr_in receiver_addr;
        socklen_t receiver_addr_len = sizeof(receiver_addr);
        memset(&receiver_addr, 0, sizeof(receiver_addr));
        *(uint16_t*)(&tx_buffer[2]) = htons(block_number);
        
        file.read((&tx_buffer[4]), TX_BUFFER_SIZE);
        int bytes_read = file.gcount();
        
        sendto(transfer_socket, tx_buffer, 4+bytes_read, 0, (const sockaddr *)&receiver_addr, sizeof(receiver_addr));
        
        for(int i=0; i<5; i++){
            if(waitForAck(transfer_socket, receiver_addr, receiver_addr_len, block_number)){
                if(receiver_addr.sin_port != client_addr.sin_port || receiver_addr.sin_addr.s_addr != client_addr.sin_addr.s_addr){
                    sendError(transfer_socket, receiver_addr, 5, "[Unauthorized client]");
                    continue;
                }
                std::cout<<"Packet ID: "<<block_number<<std::endl;
                block_number++;
                break;
            }
            else{
                std::cout<<"Retransmission... ("<<i<<")"<<std::endl;
                sendto(transfer_socket, tx_buffer, 4+bytes_read, 0, (const sockaddr *)&client_addr, sizeof(client_addr));
            }
        }
        if(bytes_read < 512) break; 
    }
}

