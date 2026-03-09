cmake_minimum_required(VERSION 3.10)

project(TftpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.cpp")

add_executable(tftpserver server/src/server.cpp server/src/main.cpp)
target_include_directories(tftpserver PRIVATE server/include)
