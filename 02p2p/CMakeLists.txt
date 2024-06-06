cmake_minimum_required(VERSION 3.2)
project(02p2p)

set(CMAKE_CXX_STANDARD 17)

add_executable(server server.cpp
        header.h)

add_executable(client client.cpp
        header.h)

