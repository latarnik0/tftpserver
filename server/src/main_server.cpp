#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iostream>

#define PORT 6969
#define BUFFER_SIZE 518
#define RESPONSE_SIZE 512
#define ADRESS_SIZE 16

int main(){
    int wSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if(wSocket<0){
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

    if(bind(wSocket, (const sockaddr *)&server_addr, sizeof(server_addr))<0){
        std::cerr<<"Bind error"<<std::endl;
        return 1;
    }
    std::cout<<"Bind executed"<<std::endl;

    char buffer[BUFFER_SIZE];
    memset(&client_addr, 0, sizeof(client_addr));
    if(recvfrom(wSocket, buffer, BUFFER_SIZE, 0, (sockaddr *)&client_addr, &client_addr_len)<0){
        std::cerr<<"Recvfrom error"<<std::endl;
        return 1;
    }

    char ip_string[ADRESS_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_string, sizeof(ip_string));
    std::cout<<"Received data from client: "<<ip_string<<":"<<ntohs(client_addr.sin_port)<<std::endl;

    char response[RESPONSE_SIZE] = "Response from server: Hello, client!";
    if(sendto(wSocket, response, strlen(response), 0, (const sockaddr *)&client_addr, sizeof(client_addr))<0){
        std::cerr<<"Sendto error"<<std::endl;
        return 1;
    }

    return 0;
}