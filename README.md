# 📤 Low-Level TFTP Implementation in C++

## Overview

**TFTP** is a lightweight, terminal-based implementation of the Trivial File Transfer Protocol (based on RFC 1350) written from scratch in **C++**. 
**This project interacts directly with Linux POSIX network sockets to handle raw UDP datagrams.**
The goal of this project is to expand knowledge and understanding of **Networks and Network Programming architecture**, specifically how to establish connectionless communication, manage byte-order conversions (Endianness), and implement a byte-level binary protocol without relying on external network libraries like `Asio`.



## Key Features 
* **Custom UDP Core:** Built entirely on system calls (`socket`, `bind`, `recvfrom`, `sendto`) without high-level abstractions.
* **Dual Binaries:** Features a unified codebase that compiles into distinct `server` and `client` executables, sharing common protocol structures.
* **Protocol Parsing:** (In Progress) Efficiently extracts 2-byte Opcodes to differentiate between Read Requests (RRQ) and Write Requests (WRQ).
* **Block File Transfer:** (Planned) Slices files into 512-byte data blocks for sequential transmission and ACK validation.
* **Multi-Client Support:** (Planned) Implements the Transfer Identifier (TID) mechanism, dynamically allocating new ephemeral ports (via multithreading) to handle multiple clients concurrently without blocking the main port 69.

## Technical Highlights

* **Linux System Programming:**
    * Understanding the **POSIX Sockets API** and connectionless UDP paradigms. 
* **C++ Development:**
    * **Memory & Byte Manipulation:** Direct manipulation of raw `char` buffers, `memset`, and pointer casting (`struct sockaddr_in` to `struct sockaddr`) for system compatibility.
    * **Endianness Management:** Utilizing `<arpa/inet.h>` functions (`htons`, `ntohs`) to safely translate between Host Byte Order and Network Byte Order (Big-Endian).
    * **Build System Automation:** Using **CMake** for a multi-target architecture. Separating `client/`, `server/`, and `common/` directories.
    * **Multithreading:** (Planned) Using `std::thread` to branch off individual file transfers to separate execution paths.

## Prerequisites
* Linux environment (Ubuntu/Xubuntu/Debian/Arch/Fedora)
* C++ Compiler (g++ or clang++) supporting C++11 or newer
* CMake (min. version 3.10) and Make
* Netcat (`nc`) for basic UDP connectivity testing

## Additional Info
* Project start date: **March 2026**
* Last update: **04.03.2026**
* Status: **in progress**