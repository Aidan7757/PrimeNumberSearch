#pragma once

#include <stdint.h>

// Schönhage-Strassen fast multiplication for very large integers
// Optimized for GPU implementation with FFT-based polynomial multiplication

#ifdef __CUDACC__
#define CUDA_DEVICE_FN __device__ __forceinline__
#else
#define CUDA_DEVICE_FN static inline
#endif

// Conditional compilation for CUDA
#ifdef __CUDACC__
#define CUDA_DEVICE_FN __device__ __forceinline__
#define CUDA_GLOBAL_FN __global__
#else
#define CUDA_DEVICE_FN static inline
#define CUDA_GLOBAL_FN
#endif

// Configuration for Schönhage-Strassen
#define SS_MIN_BITS 1024  // Minimum bit size where SS is beneficial
#define SS_BASE_CHUNK 32  // Base chunk size in bits
#define SS_MAX_CHUNKS 512  // Maximum number of chunks

// Polynomial representation for convolution
typedef struct {
    uint32_t coeffs[SS_MAX_CHUNKS];
    int num_coeffs;
    int chunk_size;  // Bits per chunk
} polynomial_t;

// FFT workspace structure
typedef struct {
    double real[SS_MAX_CHUNKS * 2];
    double imag[SS_MAX_CHUNKS * 2];
    int size;
} fft_workspace_t;

// Schönhage-Strassen specific functions
CUDA_DEVICE_FN void ss_split_number(uint64_t num, polynomial_t* poly, int chunk_size);
CUDA_DEVICE_FN void ss_polynomial_mul(polynomial_t* a, polynomial_t* b, polynomial_t* result, fft_workspace_t* ws);
CUDA_DEVICE_FN uint64_t ss_combine_coeffs(polynomial_t* poly, uint64_t mod);
CUDA_DEVICE_FN uint64_t schonhage_strassen_mul(uint64_t a, uint64_t b, uint64_t mod);

// FFT functions optimized for integer convolution
CUDA_DEVICE_FN void ss_fft(fft_workspace_t* ws, int direction);
CUDA_DEVICE_FN void ss_pointwise_multiply(fft_workspace_t* a, fft_workspace_t* b, fft_workspace_t* result);
CUDA_DEVICE_FN void ss_convolution(polynomial_t* a, polynomial_t* b, polynomial_t* result);

// Kernel for batch Schönhage-Strassen multiplication
CUDA_GLOBAL_FN void schonhage_strassen_kernel(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count);

// Host wrapper
void cuda_schonhage_strassen_multiply(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count);

// Utility functions
CUDA_DEVICE_FN int next_power_of_two(int n);
CUDA_DEVICE_FN double ss_root_of_unity(int n, int k, int direction);