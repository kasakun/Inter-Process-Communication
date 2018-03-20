cmake_minimum_required(VERSION 3.9)
project(ipc C)

set(CMAKE_C_STANDARD 99)

add_executable(ipc main.c queue.c server.c client.c)
TARGET_LINK_LIBRARIES(ipc -lrt)
