cmake_minimum_required(VERSION 3.2)
project(01p2p C)

set(CMAKE_C_STANDARD 11)

add_executable(server server.c)

add_executable(client client.c)

