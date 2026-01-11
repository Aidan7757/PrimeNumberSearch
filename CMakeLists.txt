cmake_minimum_required(VERSION 4.1)
project(PrimeNumberSearch C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(PrimeNumberSearch
        src/main.c
        src/search_methods.c
        src/search_methods.h)
