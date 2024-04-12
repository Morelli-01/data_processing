cmake_minimum_required(VERSION 3.27)
project(LZ78)

set(CMAKE_CXX_STANDARD 23)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(LZ78 lz78encode.cpp)
