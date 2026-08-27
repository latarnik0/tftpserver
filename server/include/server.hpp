#pragma once

#include <string>   
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PATH "/home/latarnik3/tftpserver/data"

constexpr int MAIN_PORT = 9069;
constexpr int ADDRESS_SIZE = 16;
constexpr int BUFFER_SIZE = 516;
constexpr int ACK_BUFFER_SIZE = 8;
constexpr int PAYLOAD_SIZE = 512;

constexpr int RRQ_OPCODE = 1;
constexpr int WRQ_OPCODE = 2;
constexpr int DATA_PACKET_OPCODE = 3;
constexpr int ACK_OPCODE = 4;
constexpr int ERROR_OPCODE = 5;

constexpr int FILE_NOT_FOUND_ERROR_CODE = 1;
constexpr int ACCESS_VIOLATION_ERROR_CODE = 2;
constexpr int UNAUTHORIZED_ERROR_CODE = 5;
constexpr int ILLEGAL_TFTP_OPERATION_ERROR_CODE = 4;

uint16_t extractOpcode(const char* buffer);
uint16_t extractBlockNumber(const char* buffer);
uint16_t extractErrorId(const char* buffer);
std::string extractTransferMode(const char* buffer, std::string& file_name);
void newPort(int &new_socket);
void createSocket();
void bindSocket(int main_sock, const sockaddr_in& server_address);
bool waitForInitialPacket(int main_socket, char* rx_buffer, sockaddr_in& client_addr, socklen_t& client_addr_len);
sockaddr_in makeServerAddress();
void sendError(int socket, sockaddr_in &target_addr, uint16_t error_code, const std::string& error_msg);
bool waitForAck(int socket, sockaddr_in &sender_addr, socklen_t &sender_addr_len, uint16_t expected_block);
void waitForData(char (&rx_buffer)[BUFFER_SIZE], char (&tx_buffer)[BUFFER_SIZE], sockaddr_in &client_addr, socklen_t &client_addr_len);
void sendData(char (&tx_buffer)[BUFFER_SIZE], char (&rx_buffer)[BUFFER_SIZE], sockaddr_in &client_addr);
