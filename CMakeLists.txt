cmake_minimum_required(VERSION 3.17)
project(P2P C)

set(CMAKE_C_STANDARD 11)
link_libraries(crypt)
add_executable(P2P main.c client.c server.c functions.h)