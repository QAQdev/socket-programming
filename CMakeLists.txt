cmake_minimum_required(VERSION 3.20)
project(socket_programming)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(socket_programming
        client.cc
        Message.cpp
#        server.cc
        util.hh)
