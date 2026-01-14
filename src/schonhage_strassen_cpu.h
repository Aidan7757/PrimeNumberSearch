#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// CPU-only versions of Schönhage-Strassen for OpenMP
#ifndef SS_CPU_MIN_BITS
#define SS_CPU_MIN_BITS 1024
#endif
#ifndef SS_CPU_BASE_CHUNK
#define SS_CPU_BASE_CHUNK 32
#endif
#ifndef SS_CPU_MAX_CHUNKS
#define SS_CPU_MAX_CHUNKS 512
#endif

#ifndef SS_CPU_BASE_CHUNK
#define SS_CPU_BASE_CHUNK 32
#endif

#ifndef SS_CPU_MAX_CHUNKS
#define SS_CPU_MAX_CHUNKS 512
#endif

// CPU polynomial representation
typedef struct {
    uint32_t coeffs[SS_CPU_MAX_CHUNKS];
    int num_coeffs;
    int chunk_size;
} cpu_polynomial_t;

// CPU FFT workspace
typedef struct {
    double real[SS_CPU_MAX_CHUNKS * 2];
    double imag[SS_CPU_MAX_CHUNKS * 2];
    int size;
} cpu_fft_workspace_t;

// Function declarations
static void cpu_ss_split_number(uint64_t num, cpu_polynomial_t* poly);
static void ss_pointwise_multiply(cpu_polynomial_t* a, cpu_polynomial_t* b, cpu_polynomial_t* result);
static void ss_polynomial_mul(cpu_polynomial_t* a, cpu_polynomial_t* b, cpu_polynomial_t* result);
static void cpu_ss_fft(cpu_fft_workspace_t* ws, int direction);
static void ss_convolution(cpu_fft_workspace_t* a, cpu_fft_workspace_t* b, cpu_fft_workspace_t* result);

// CPU Schönhage-Strassen functions
static uint64_t cpu_schonhage_strassen_mul_impl(uint64_t a, uint64_t b, uint64_t mod);

// Wrapper that matches CUDA interface
uint64_t schonhage_strassen_mul(uint64_t a, uint64_t b, uint64_t mod);