#pragma once

#include <stdint.h>

// CPU-only versions of FFT multiplication for OpenMP
// These provide CPU-optimized fallbacks when CUDA is not available

#ifndef FFT_CPU_BITS
#define FFT_CPU_BITS 32
#endif

#ifndef FFT_CPU_MAX_COEFFS
#define FFT_CPU_MAX_COEFFS 256
#endif

// Simple CPU double-precision complex number
typedef struct {
    double real;
    double imag;
} cpu_complex_t;

// CPU large integer representation
typedef struct {
    uint64_t coeffs[FFT_CPU_MAX_COEFFS];
    int num_coeffs;
    int bit_shift;
} cpu_large_int_t;

// CPU FFT functions (simplified versions for demonstration)
void cpu_fft_pointwise_mul(cpu_complex_t* a, cpu_complex_t* b, cpu_complex_t* result, int n);
void cpu_complex_add(cpu_complex_t a, cpu_complex_t b, cpu_complex_t* result);
void cpu_complex_mul(cpu_complex_t a, cpu_complex_t b, cpu_complex_t* result);

// Main CPU FFT multiplication function
uint64_t cpu_fft_multiply_impl(uint64_t a, uint64_t b);

// Wrapper that matches CUDA interface
void fft_multiply(uint64_t a, uint64_t b, uint64_t* result);