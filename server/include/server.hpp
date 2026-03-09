#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PATH "tftpserver/data"

constexpr int MAIN_PORT = 6969;
constexpr int TX_BUFFER_SIZE = 512;
constexpr int RX_BUFFER_SIZE = 516; 
constexpr int ADDRESS_SIZE = 16;

uint16_t extractOpcode(const char* buffer);
uint16_t extractBlockNumber(const char* buffer);
uint16_t extractErrorId(const char* buffer);

void newPort(int &new_socket);
bool waitForAck(int socket, sockaddr_in &sender_addr, socklen_t &sender_addr_len, uint16_t expected_block);
void waitForData(char (&rx_buffer)[RX_BUFFER_SIZE], char (&tx_buffer)[TX_BUFFER_SIZE], sockaddr_in &client_addr, socklen_t &client_addr_len);
void sendData(char (&tx_buffer)[TX_BUFFER_SIZE], char (&rx_buffer)[RX_BUFFER_SIZE], sockaddr_in &client_addr);