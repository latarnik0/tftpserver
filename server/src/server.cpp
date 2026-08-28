#include "server/include/server.hpp"
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <algorithm>

// further improvments: add logging comms system

uint16_t extractOpcode(const char* buffer)
{
    return (static_cast<uint16_t>(static_cast<unsigned char>(buffer[0])) << 8) | 
            static_cast<uint16_t>(static_cast<unsigned char>(buffer[1]));
}

uint16_t extractBlockNumber(const char* buffer)
{
    return (static_cast<uint16_t>(static_cast<unsigned char>(buffer[2])) << 8) | 
            static_cast<uint16_t>(static_cast<unsigned char>(buffer[3]));
}

uint16_t extractErrorId(const char* buffer)
{
    return (static_cast<uint16_t>(static_cast<unsigned char>(buffer[2])) << 8) | 
            static_cast<uint16_t>(static_cast<unsigned char>(buffer[3]));
}

void sendError(int socket, const sockaddr_in &target_addr, uint16_t error_code, const std::string& error_msg)
{
    char error_buffer[BUFFER_SIZE];
    *(uint16_t*)(&error_buffer[0]) = htons(5); // ERROR_OPCODE
    *(uint16_t*)(&error_buffer[2]) = htons(error_code);
    std::strcpy(&error_buffer[4], error_msg.c_str());
    int packet_size = 4 + error_msg.length() + 1;
    
    sendto(socket, error_buffer, packet_size, 0, (const sockaddr *)&target_addr, sizeof(target_addr));
}

bool checkFileName(const std::string& filename, const sockaddr_in& client_addr, int socket)
{
    if (filename.empty()) {
        sendError(socket, client_addr, ACCESS_VIOLATION_ERROR_CODE, "[Access Violation: Empty filename]");
        return false; 
    }

    std::error_code ec;
    std::filesystem::path base_dir = std::filesystem::weakly_canonical(std::filesystem::path(PATH), ec);
    std::filesystem::path requested_path = std::filesystem::weakly_canonical(base_dir / filename, ec);

    auto it_base = base_dir.begin();
    auto it_req = requested_path.begin();
    
    while (it_base != base_dir.end() && it_req != requested_path.end() && *it_base == *it_req) {
        ++it_base;
        ++it_req;
    }

    if (it_base != base_dir.end()) {
        sendError(socket, client_addr, ACCESS_VIOLATION_ERROR_CODE, "[Access Violation: Path Traversal]");
        return false;
    }

    return true;
}

bool parseRequest(const char* buffer, size_t max_buffer_size, std::string& out_filename, std::string& out_mode)
{
    if (max_buffer_size < 6) return false;
    size_t filename_len = strnlen(&buffer[2], max_buffer_size - 2);
    if (filename_len == max_buffer_size - 2) return false; 

    out_filename = std::string(&buffer[2], filename_len);

    size_t mode_offset = 2 + filename_len + 1;
    if (mode_offset >= max_buffer_size) return false;

    size_t mode_len = strnlen(&buffer[mode_offset], max_buffer_size - mode_offset);
    if (mode_len == max_buffer_size - mode_offset) return false;

    out_mode = std::string(&buffer[mode_offset], mode_len);
    std::transform(out_mode.begin(), out_mode.end(), out_mode.begin(), ::tolower);

    if (out_mode != "netascii" && out_mode != "octet") {
        out_mode = "octet";
    }
    return true;
}

void createSocket(int& new_socket)
{
    new_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (new_socket < 0) 
    {
        std::cerr << "Socket creation error\n";
    }
}

void createNewAddress(sockaddr_in& new_addr, int port)
{
    memset(&new_addr, 0, sizeof(new_addr));
    new_addr.sin_family = AF_INET;
    new_addr.sin_port = htons(port);
    new_addr.sin_addr.s_addr = htonl(INADDR_ANY);
}

bool bindSocket(int new_socket, const sockaddr_in& address)
{
    if (bind(new_socket, (const sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind error\n";
        return false;
    }
    return true;
}

bool createNewPort(int& new_socket, sockaddr_in& new_addr)
{
    createSocket(new_socket);
    if (new_socket < 0) return false;
    createNewAddress(new_addr, 0);
    return bindSocket(new_socket, new_addr);
}

void handleRecvMode(const std::string& transfer_mode, int& payload_size, bool& edge_case, char (&tx_buffer)[BUFFER_SIZE], std::ifstream& file)
{
    if(transfer_mode == "netascii")
    {
        char c;
        if(edge_case)
        {
            tx_buffer[4+payload_size] = '\n';
            payload_size++;
            edge_case = false;
        }
        while(file.get(c))
        {
            if(c == '\n')
            {
                if(payload_size == PAYLOAD_SIZE - 1)
                {
                    tx_buffer[4+payload_size] = '\r';
                    payload_size++;
                    edge_case = true;
                    break;
                }
                else
                {
                    tx_buffer[4+payload_size] = '\r';
                    payload_size++;
                    tx_buffer[4+payload_size] = '\n';
                    payload_size++;
                }
            }
            else if(c == '\r')
            {
                tx_buffer[4+payload_size] = '\r';
                payload_size++;
                tx_buffer[4+payload_size] = '\0';
                payload_size++;
            }
            else
            {
                tx_buffer[4+payload_size] = c;
                payload_size++;
            }

            if(payload_size >= PAYLOAD_SIZE)
                break;
        }
    }
    else
    {
        file.read((&tx_buffer[4]), PAYLOAD_SIZE);
        payload_size = file.gcount();
    }
}

void handleSendingMode(const std::string& transfer_mode, bool& edge_case, int bytes_recv, std::ofstream& file, const char* rx_buffer)
{
    if(transfer_mode == "netascii")
    {
        bool prev_r = false;
        for(int i = 4; i < bytes_recv; i++)
        {
            if(edge_case)
            {
                if(rx_buffer[i] == '\n') file.put(rx_buffer[i]);
                else if(rx_buffer[i] == '\0') file.put('\r');
                else { file.put('\r'); file.put(rx_buffer[i]); }
                edge_case = false;
                continue;
            }

            if(prev_r)
            {
                if(rx_buffer[i] == '\n') { file.put('\n'); prev_r = false; continue; }
                else if(rx_buffer[i] == '\0') { file.put('\r'); prev_r = false; continue; }
                else { file.put('\r'); }
                prev_r = false;
            }

            if(rx_buffer[i] == '\r')
            {
                prev_r = true;
                if(i == bytes_recv - 1)
                {
                    edge_case = true;
                    break;
                }
            }
            else
            {
                file.put(rx_buffer[i]);
            }
        }
    }
    else
    {
        file.write(&rx_buffer[4], bytes_recv - 4);
    }
}

bool waitForAck(int socket, sockaddr_in &sender_addr, socklen_t &sender_addr_len, uint16_t expected_block)
{
    char ack_buffer[ACK_BUFFER_SIZE];
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while(true)
    {
        int bytes_received = recvfrom(socket, ack_buffer, sizeof(ack_buffer), 0, (sockaddr *)&sender_addr, &sender_addr_len);
    
        if(bytes_received < MIN_PACKET_SIZE)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return false; 
            return false;
        }

        uint16_t ack_opcode = extractOpcode(ack_buffer);
        uint16_t ack_block_num = extractBlockNumber(ack_buffer);
    
        if(ack_opcode == ACK_OPCODE && ack_block_num == expected_block)
        {
            return true;
        }
        else if(ack_opcode == ACK_OPCODE && ack_block_num != expected_block)
        {
            continue;
        }
        else if(ack_opcode == ERROR_OPCODE)
        {
            // error log
            return false;
        }
        else
        {
            return false;
        }
    }
}

void waitForData(char (&rx_buffer)[BUFFER_SIZE], char (&tx_buffer)[BUFFER_SIZE], sockaddr_in &client_addr, socklen_t client_addr_len)
{
    int receive_socket;
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    if(!createNewPort(receive_socket, sender_addr)) return;

    std::string file_name, transfer_mode;
    if(!parseRequest(rx_buffer, BUFFER_SIZE, file_name, transfer_mode)) {
        sendError(receive_socket, client_addr, ILLEGAL_TFTP_OPERATION_ERROR_CODE, "[Malformed Request]");
        close(receive_socket);
        return;
    }

    if(!checkFileName(file_name, client_addr, receive_socket))
    {
        close(receive_socket);
        return; 
    }

    std::filesystem::path file_path = std::filesystem::path(PATH) / file_name;
    std::ofstream file;
    if(transfer_mode == "octet") file.open(file_path, std::ios::binary);
    else file.open(file_path);

    if(!file.is_open())
    {
        sendError(receive_socket, client_addr, ACCESS_VIOLATION_ERROR_CODE, "[Could not open file for writing]");
        close(receive_socket);
        return;
    }

    uint16_t ack_block_number = 0;   
    *(uint16_t*)(&tx_buffer[0]) = htons(ACK_OPCODE);
    *(uint16_t*)(&tx_buffer[2]) = htons(ack_block_number);

    struct timeval tv; tv.tv_sec = 3; tv.tv_usec = 0;
    setsockopt(receive_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sendto(receive_socket, tx_buffer, ACK_BUFFER_SIZE, 0, (const sockaddr *)&client_addr, sizeof(client_addr));

    bool edge_case = false;

    while(true)
    {
        int wait_limit = MAX_RETRANSMISSION_LIMIT;
        int bytes_recv;
        
        while(wait_limit > 0) {
            bytes_recv = recvfrom(receive_socket, rx_buffer, BUFFER_SIZE, 0, (sockaddr *)&sender_addr, &sender_addr_len);

            if(bytes_recv < 0) {
                wait_limit--;
                sendto(receive_socket, tx_buffer, ACK_BUFFER_SIZE, 0, (const sockaddr *)&client_addr, sizeof(client_addr));
            } else {
                break;
            }
        }

        if (wait_limit <= 0) break;

        if(sender_addr.sin_port != client_addr.sin_port || sender_addr.sin_addr.s_addr != client_addr.sin_addr.s_addr)
        {
            sendError(receive_socket, sender_addr, UNAUTHORIZED_ERROR_CODE, "[Unauthorized client/TID]");
            continue;
        }
        
        if (bytes_recv < 4) continue;

        uint16_t recv_opcode = extractOpcode(rx_buffer);
        uint16_t recv_block_number = extractBlockNumber(rx_buffer);

        if(recv_opcode == ERROR_OPCODE) {
            break;
        }
        else if(recv_opcode != DATA_PACKET_OPCODE) {
            sendError(receive_socket, client_addr, ILLEGAL_TFTP_OPERATION_ERROR_CODE, "[Illegal TFTP operation]");
            break;
        }

        if (recv_block_number == (uint16_t)(ack_block_number + 1)) {
            handleRecvMode(transfer_mode, edge_case, bytes_recv, file, rx_buffer);
            ack_block_number = recv_block_number;
        }

        *(uint16_t*)(&tx_buffer[2]) = htons(ack_block_number);
        sendto(receive_socket, tx_buffer, ACK_BUFFER_SIZE, 0, (const sockaddr *)&client_addr, sizeof(client_addr));

        if(bytes_recv < BUFFER_SIZE) break;
    }
    
    file.close();
    close(receive_socket);
}

void sendData(char (&tx_buffer)[BUFFER_SIZE], char (&rx_buffer)[BUFFER_SIZE], sockaddr_in &client_addr)
{
    int transfer_socket;
    struct sockaddr_in receiver_addr;
    socklen_t receiver_addr_len = sizeof(receiver_addr);

    if(!createNewPort(transfer_socket, receiver_addr)) return;

    std::string file_name, transfer_mode;
    if(!parseRequest(rx_buffer, BUFFER_SIZE, file_name, transfer_mode)) {
        sendError(transfer_socket, client_addr, ILLEGAL_TFTP_OPERATION_ERROR_CODE, "[Malformed Request]");
        close(transfer_socket);
        return;
    }

    if(!checkFileName(file_name, client_addr, transfer_socket)) {
        close(transfer_socket);
        return;
    }

    std::filesystem::path file_path = std::filesystem::path(PATH) / file_name; 
    
    std::ifstream file;
    if(transfer_mode == "octet") file.open(file_path, std::ios::binary);
    else file.open(file_path);

    if(!file.is_open())
    {
        sendError(transfer_socket, client_addr, FILE_NOT_FOUND_ERROR_CODE, "[File not found]");
        close(transfer_socket);
        return;
    }

    uint16_t block_number = 1;
    *(uint16_t*)(&tx_buffer[0]) = htons(DATA_PACKET_OPCODE);

    bool edge_case = false;
    
    while(true)
    {
        *(uint16_t*)(&tx_buffer[2]) = htons(block_number);
        int payload_size = 0;
        
        handleSendingMode(transfer_mode, payload_size, edge_case, tx_buffer, file);

        bool acked = false;
        for(int i=0; i<5; i++)
        {
            sendto(transfer_socket, tx_buffer, 4 + payload_size, 0, (const sockaddr *)&client_addr, sizeof(client_addr));
            
            if(waitForAck(transfer_socket, receiver_addr, receiver_addr_len, block_number))
            {
                if(receiver_addr.sin_port != client_addr.sin_port || receiver_addr.sin_addr.s_addr != client_addr.sin_addr.s_addr)
                {
                    sendError(transfer_socket, receiver_addr, UNAUTHORIZED_ERROR_CODE, "[Unauthorized client/TID]");
                    continue;
                }
                acked = true;
                block_number++;
                break;
            }
            std::cout << "Retransmission... (" << i+1 << ")\n";
        }
        
        if (!acked) {
            std::cerr << "Timeout, transfer aborted.\n";
            break;
        }

        if(payload_size < PAYLOAD_SIZE) break;
    }
    
    file.close();
    close(transfer_socket);
}