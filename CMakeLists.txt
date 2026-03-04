cmake_minimum_required(VERSION 3.10)

project(TftpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(common/include)

file(GLOB SOURCES "src/*.cpp")


add_executable(server server/src/main_server.cpp)
add_executable(client client/src/main_client.cpp)
