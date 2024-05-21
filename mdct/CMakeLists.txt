cmake_minimum_required(VERSION 3.28)
project(mdct)

set(CMAKE_CXX_STANDARD 26)


add_executable(mdct
#        mdct.cpp
#        error.h
#        error.cpp
        main.cpp
)

#target_link_libraries(mdct PRIVATE "-Wl,--stack,160000000")
