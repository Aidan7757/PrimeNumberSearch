CC = gcc-15
CFLAGS = -fopenmp search_methods_openmp_test.c ../src/search_methods_openmp.c ../src/utils.c ../src/schonhage_strassen_cpu.c ../src/fft_multiply_cpu.c  -o tests.exe
