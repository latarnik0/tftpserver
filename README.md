# Low-Level TFTP Implementation in C++

## Overview

**TFTP** is terminal-based implementation of the Trivial File Transfer Protocol (based on RFC 1350) written from scratch in **C++**. 
**This project interacts directly with Linux POSIX network sockets.**
The goal of this project is to expand knowledge and understanding of **Networks and Network Programming architecture**, specifically how to establish secure and connectionless communication, manage byte-order conversions (Endianness), and implement a byte-level binary protocol without relying on external network libraries.


## Key Features 
* **Custom UDP Core:** Built entirely on system calls (`socket`, `bind`, `recvfrom`, `sendto`, `setsockopt`) without high-level abstractions.
* **Protocol Parsing:** Efficiently extracts 2-byte Opcodes to differentiate between Read Requests (RRQ) and Write Requests (WRQ).
* **Transfer modes:** Efficiently handling the two modes of data transfer: `octet` (binary) and `netascii`.
* **Block File Transfer:** Slices files into 512-byte data blocks for sequential transmission and ACK validation.
* **Multi-Client Support:** Implements the Transfer Identifier (TID) mechanism, dynamically allocating new ephemeral ports to handle multiple clients concurrently without blocking the main port 69.


## Technical Highlights

* **Linux System Programming:**
    * Understanding the **POSIX Sockets API** and connectionless UDP paradigms.
    * Deeper understanding of Linux kernel system calls: `socket`, `bind`, `recvfrom`, `sendto`, `setsockopt`.
* **C++ Development:**
    * **Memory & Byte Manipulation:** Direct manipulation of raw `char` buffers, `memset`, and pointer casting (`struct sockaddr_in` to `struct sockaddr`) for system compatibility.
    * **Endianness Management:** Utilizing `<arpa/inet.h>` functions (`htons`, `ntohs`) to safely translate between Host Byte Order and Network Byte Order (Big-Endian).
    * **Build System Automation:** Using **CMake** to build project.
    * **Multithreading:** (Planned)
    * **Custom Block Size:** (Planned)
    * **Transfer Size:** (Planned)
    * **Custom Timeout:** (Planned)
* **Security and Reliability:**
    * **Directory Traversal:** Client can only request and write files from dedicated "working" folder `data`.
    * **Buffer Overflow:** Strictly controlled buffer size preventing critical errors (e.g Segmentation Fault).
    * **Intruder attack:** Verifing, if the client who is supposed to receive or transmit data is the client who made the original request.
    * **Timeout handling:** Using timeouts (`setsockopt` with `SO_RCVTIMEO`), preventing early retransmissions.
    * **Edge Cases handling:** Preventing data damage when whitespace character sequence is cut in half (e.g `\r` was last byte in buffer).

## Prerequisites
* Linux environment (Ubuntu/Xubuntu/Debian/Arch/Fedora)
* C++ Compiler (g++ or clang++) supporting C++11 or newer
* CMake (min. version 3.10) and Make
* Netcat (`nc`) for basic UDP connectivity testing

## Additional Info
* Project start date: **March 2026**
* Last update: **27.03.2026**
* Status: **in progress**
