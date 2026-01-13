#pragma once

#include <stdint.h>

// CUDA FFT-based fast multiplication for large integers
// Implements Schönhage-Strassen algorithm for very large numbers

#ifdef __CUDACC__
#define CUDA_DEVICE_FN __device__ __forceinline__
#define CUDA_GLOBAL_FN __global__
#else
#define CUDA_DEVICE_FN static inline
#define CUDA_GLOBAL_FN
#endif

// Configuration for FFT-based multiplication
#ifndef FFT_BITS
#define FFT_BITS 32
#endif

#ifndef FFT_MAX_COEFFS
#define FFT_MAX_COEFFS 256
#endif

// Complex number structure for FFT
typedef struct {
    double real;
    double imag;
} complex_t;

// Large integer representation for FFT multiplication
typedef struct {
    uint64_t coeffs[FFT_MAX_COEFFS];
    int num_coeffs;
    int bit_shift;
} large_int_t;

// CUDA device functions for FFT-based multiplication
CUDA_DEVICE_FN void fft_radix2(complex_t* data, int n, int direction);
CUDA_DEVICE_FN void fft_pointwise_mul(complex_t* a, complex_t* b, complex_t* result, int n);
CUDA_DEVICE_FN void fft_multiply(uint64_t a, uint64_t b, uint64_t* result);
CUDA_DEVICE_FN uint64_t cuda_fast_mod_mul(uint64_t a, uint64_t b, uint64_t mod);
CUDA_DEVICE_FN void large_int_init(large_int_t* num, uint64_t value);
CUDA_DEVICE_FN void large_int_split_coeffs(large_int_t* num);
CUDA_DEVICE_FN void large_int_from_coeffs(large_int_t* num, uint64_t* result);

// Main fast multiplication kernels
CUDA_GLOBAL_FN void fft_multiply_kernel(const uint64_t* a, const uint64_t* b, uint64_t* result, int count);
CUDA_GLOBAL_FN void fast_mod_mul_kernel(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count);

// Host wrapper functions
void cuda_fft_multiply(const uint64_t* a, const uint64_t* b, uint64_t* result, int count);
void cuda_fast_mod_multiply(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count);

// Utility functions
CUDA_DEVICE_FN complex_t complex_add(complex_t a, complex_t b);
CUDA_DEVICE_FN complex_t complex_sub(complex_t a, complex_t b);
CUDA_DEVICE_FN complex_t complex_mul(complex_t a, complex_t b);
CUDA_DEVICE_FN complex_t complex_exp(double angle);
CUDA_DEVICE_FN void bit_reverse(complex_t* data, int n);
CUDA_DEVICE_FN int reverse_bits(int num, int bits);