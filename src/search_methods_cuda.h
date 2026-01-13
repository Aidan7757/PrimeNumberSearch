#pragma once
#include <stdbool.h>
#include <stddef.h>

// Conditional CUDA compilation
#ifdef HAVE_CUDA
#include <cuda_runtime.h>
#define CUDA_DEVICE_FN __device__ __forceinline__
#define CUDA_GLOBAL_FN __global__
#define CUDA_HOST_FN __host__
#else
// Fallback definitions when CUDA is not available
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Mock CUDA attributes
#define __device__
#define CUDA_GLOBAL_FN
#define CUDA_HOST_FN
#define CUDA_DEVICE_FN static inline

// Mock CUDA types
typedef struct {
    unsigned int x, y, z;
} dim3;

// Mock CUDA functions
#define cudaMemcpyHostToDevice 1
#define cudaMemcpyDeviceToHost 2
inline int cudaMalloc(void** ptr, size_t size) { (void)ptr; (void)size; return -1; }
inline int cudaFree(void* ptr) { (void)ptr; return -1; }
inline int cudaMemcpy(void* dst, const void* src, size_t count, int kind) { (void)dst; (void)src; (void)count; (void)kind; return -1; }
inline int cudaSetDevice(int device) { (void)device; return -1; }
inline void cudaDeviceSynchronize() {}
extern dim3 blockDim;
extern dim3 blockIdx;
extern dim3 threadIdx;
#endif

#include "../src/models.h"

// CUDA device functions for primality testing
__device__ bool cuda_naive_check(long long potential_prime);
__device__ bool cuda_miller_rabin(long long potential_prime, int num_rounds, unsigned int seed);
__device__ bool cuda_fermat(long long potential_prime, int num_rounds, unsigned int seed);
__device__ bool cuda_gauss_euler(long long potential_prime);
__device__ bool cuda_mr_ge(long long potential_prime);

// CUDA utility functions
__device__ unsigned long long cuda_mod_mul(unsigned long long a, unsigned long long b, unsigned long long mod);
__device__ unsigned long long cuda_mod_pow(unsigned long long base, unsigned long long exp, unsigned long long mod);
__device__ long cuda_factor_out_twos(unsigned long long potential_prime, unsigned long long* d);

// CUDA kernel for parallel prime search
CUDA_GLOBAL_FN void cuda_prime_search_kernel(const long long* numbers, bool* results, int count, int method, int num_rounds);

// Host wrapper functions
void cuda_prime_search(const long long* numbers, bool* results, int count, int method, int num_rounds);
void cuda_initialize();
bool cuda_is_available();