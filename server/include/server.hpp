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
constexpr int MIN_PACKET_SIZE = 4;

constexpr int RRQ_OPCODE = 1;
constexpr int WRQ_OPCODE = 2;
constexpr int DATA_PACKET_OPCODE = 3;
constexpr int ACK_OPCODE = 4;
constexpr int ERROR_OPCODE = 5;

constexpr int FILE_NOT_FOUND_ERROR_CODE = 1;
constexpr int ACCESS_VIOLATION_ERROR_CODE = 2;
constexpr int UNAUTHORIZED_ERROR_CODE = 5;
constexpr int ILLEGAL_TFTP_OPERATION_ERROR_CODE = 4;

constexpr int MAX_RETRANSMISSION_LIMIT = 4;

uint16_t extractOpcode(const char* buffer);
uint16_t extractBlockNumber(const char* buffer);
uint16_t extractErrorId(const char* buffer);
void sendError(int socket, const sockaddr_in &target_addr, uint16_t error_code, const std::string& error_msg);
bool checkFileName(const std::string& filename, const sockaddr_in& client_addr, int socket);
bool parseRequest(const char* buffer, size_t max_buffer_size, std::string& out_filename, std::string& out_mode);
void createSocket(int& new_socket);
void createNewAddress(sockaddr_in& new_addr, int port);
bool bindSocket(int new_socket, const sockaddr_in& address);
bool createNewPort(int& new_socket, sockaddr_in& new_addr);
void handleRecvMode(const std::string& transfer_mode, int& payload_size, bool& edge_case, char (&tx_buffer)[BUFFER_SIZE], std::ifstream& file);
void handleSendingMode(const std::string& transfer_mode, bool& edge_case, int bytes_recv, std::ofstream& file, const char* rx_buffer);
bool waitForInitialPacket(int main_socket, char* rx_buffer, sockaddr_in& client_addr, socklen_t& client_addr_len);
bool waitForAck(int socket, sockaddr_in &sender_addr, socklen_t &sender_addr_len, uint16_t expected_block);
void waitForData(char (&rx_buffer)[BUFFER_SIZE], char (&tx_buffer)[BUFFER_SIZE], sockaddr_in &client_addr, socklen_t client_addr_len);
void sendData(char (&tx_buffer)[BUFFER_SIZE], char (&rx_buffer)[BUFFER_SIZE], sockaddr_in &client_addr);