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

#define PORT 6969
#define BUFFER_SIZE 518
#define RESPONSE_SIZE 512
#define ADRESS_SIZE 16
#define OPCODE_SIZE 4
#define PATH "[enter server data dir path here]"

void extractOpcode(const char* buffer, char* opcode){
    opcode[0] = buffer[0];
    opcode[1] = buffer[1];
    opcode[2] = buffer[2];
    opcode[3] = buffer[3];
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

    char buffer[BUFFER_SIZE];
    memset(&client_addr, 0, sizeof(client_addr));
    if(recvfrom(w_socket, buffer, BUFFER_SIZE, 0, (sockaddr *)&client_addr, &client_addr_len)<0){
        std::cerr<<"Recvfrom error"<<std::endl;
        return 1;
    }

    char ip_string[ADRESS_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_string, sizeof(ip_string));
    std::cout<<"Connected client: "<<ip_string<<":"<<ntohs(client_addr.sin_port)<<std::endl;

    char opcode[OPCODE_SIZE];
    extractOpcode(buffer, opcode);
    u_int16_t opcode_value = (static_cast<u_int16_t>(opcode[0]) << 8) | static_cast<u_int16_t>(opcode[1]);
    std::cout<<"Received opcode: "<<opcode_value<<std::endl;

    if(opcode_value == 1){
        std::cout<<"RRQ"<<std::endl;
        std::string file_name(&buffer[2]);
        std::filesystem::path file_path = std::filesystem::path(PATH) / file_name; 
        std::ifstream file(file_path, std::ios::binary);

        if(file){
            std::cout<<"[FILE FOUND]"<<std::endl;
        }
        else{
            std::cout<<"[FILE NOT FOUND]"<<std::endl;
        }

    }else if(opcode_value == 2){
        std::cout<<"WRQ"<<std::endl;
    }

    char response[RESPONSE_SIZE] = "Connection established";
    if(sendto(w_socket, response, strlen(response), 0, (const sockaddr *)&client_addr, sizeof(client_addr))<0){
        std::cerr<<"Sendto error"<<std::endl;
        return 1;
    }

    return 0;
}