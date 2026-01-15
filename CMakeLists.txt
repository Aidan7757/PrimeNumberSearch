cmake_minimum_required(VERSION 3.20)
project(PrimeNumberSearch C)

set(CMAKE_C_STANDARD 11)

# Use Homebrew GCC (Apple Silicon location)
set(CMAKE_C_COMPILER /opt/homebrew/bin/gcc-15)

include_directories(src)

add_executable(PrimeNumberSearch
        src/search_methods_openmp.c
        src/search_methods_openmp.h
        src/utils.h
        src/utils.c
        src/models.c
        src/models.h
        tests/search_methods_openmp_test.c
        src/fft_multiply_cpu.c
        src/fft_multiply_cpu.h
        src/schonhage_strassen_cpu.c
        src/schonhage_strassen_cpu.h
        src/bn.c
        src/bn.h
)

# Add OpenMP flags directly
target_compile_options(PrimeNumberSearch PRIVATE -fopenmp)
target_link_options(PrimeNumberSearch PRIVATE -fopenmp)