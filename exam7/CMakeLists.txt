cmake_minimum_required(VERSION 3.28)
project(exam7)

set(CMAKE_CXX_STANDARD 23)

add_executable(exam7 json.cpp
        mat.h
        ppm.h
        ppm.cpp
)
