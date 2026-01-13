#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#ifdef HAVE_CUDA
#include <cuda_runtime.h>
#endif

#include "search_methods_cuda.h"
#include "utils.h"

#ifndef HAVE_CUDA
// Mock CUDA global variables
dim3 blockDim = {256, 1, 1};
dim3 blockIdx = {0, 0, 0};
dim3 threadIdx = {0, 0, 0};
#endif

// CUDA device utility functions with Schönhage-Strassen optimization
__device__ unsigned long long cuda_mod_mul(unsigned long long a, unsigned long long b, unsigned long long mod) {
    // Use traditional method only - FFT and Schönhage-Strassen disabled
    a %= mod;
    b %= mod;
    
    #if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 300
        // Use 128-bit multiplication on modern CUDA
        unsigned __int128 temp = (unsigned __int128)a * b;
        return (unsigned long long)(temp % mod);
    #else
        // Russian peasant algorithm for older architectures
        unsigned long long res = 0;
        a %= mod;
        
        while (b > 0) {
            if (b & 1) {
                if (res >= mod - a) {
                    res = res + a - mod;
                } else {
                    res = res + a;
                }
            }
            
            if (a >= mod - a) {
                a = a + a - mod;
            } else {
                a = a + a;
            }
            
            b >>= 1;
        }
        return res;
    #endif
}


        a = (a + a) % mod;
        b >>= 1;
    }
    return res;
}

__device__ unsigned long long cuda_mod_pow(unsigned long long base, unsigned long long exp, unsigned long long mod) {
    unsigned long long res = 1;
    base %= mod;
    
    while (exp > 0) {
        if (exp & 1) {
            res = cuda_mod_mul(res, base, mod);
        }
        base = cuda_mod_mul(base, base, mod);
        exp >>= 1;
    }
    return res;
}

__device__ long cuda_factor_out_twos(unsigned long long potential_prime, unsigned long long* d) {
    unsigned long long value = potential_prime - 1;
    long s = 0;
    
    while (value % 2 == 0) {
        value /= 2;
        s += 1;
    }
    *d = value;
    return s;
}

// CUDA device primality test functions
__device__ bool cuda_naive_check(long long potential_prime) {
    if (potential_prime <= 1) return false;
    if (potential_prime == 2) return true;
    if (potential_prime % 2 == 0) return false;
    
    for (long long i = 3; i * i <= potential_prime; i += 2) {
        if (potential_prime % i == 0) return false;
    }
    return true;
}

__device__ bool cuda_miller_rabin(long long potential_prime, int num_rounds, unsigned int seed) {
    if (potential_prime <= 1) return false;
    if (potential_prime == 2 || potential_prime == 3) return true;
    if (potential_prime % 2 == 0) return false;
    
    unsigned long long d;
    const long s = cuda_factor_out_twos(potential_prime, &d);
    
    for (int round = 0; round < num_rounds; ++round) {
        // Simple pseudo-random generator for CUDA
        seed = seed * 1103515245 + 12345;
        const unsigned long long a = (seed % (potential_prime - 3)) + 2;
        
        unsigned long long x = cuda_mod_pow(a, d, potential_prime);
        
        if (x == 1 || x == potential_prime - 1) {
            continue;
        }
        
        bool composite = true;
        for (long j = 0; j < s - 1; ++j) {
            x = cuda_mod_pow(x, 2, potential_prime);
            if (x == potential_prime - 1) {
                composite = false;
                break;
            }
        }
        
        if (composite) {
            return false;
        }
    }
    
    return true;
}

__device__ bool cuda_fermat(long long potential_prime, int num_rounds, unsigned int seed) {
    if (potential_prime == 1) return false;
    if (potential_prime == 2 || potential_prime == 3) return true;
    if (potential_prime % 2 == 0) return false;
    
    for (int i = 2; i < num_rounds; ++i) {
        seed = seed * 1103515245 + 12345;
        const unsigned long long a = (seed % (potential_prime - 3)) + 2;
        const unsigned long long result = cuda_mod_pow(a, potential_prime - 1, potential_prime);
        if (result != 1) return false;
    }
    return true;
}

__device__ bool cuda_gauss_euler(long long potential_prime) {
    if (potential_prime == 2) return true;
    if (!(potential_prime & 1) || potential_prime < 2) return false;
    
    const unsigned long long n = (unsigned long long)potential_prime;
    
    // Euler criterion for 2
    unsigned long long t = cuda_mod_pow(2, (n - 1) / 2, n);
    if ((n % 8 == 1 || n % 8 == 7) && t != 1) {
        return false;
    }
    if ((n % 8 == 3 || n % 8 == 5) && t != n - 1) {
        return false;
    }
    
    // Check powers near sqrt(n) and sqrt(n/2)
    for (int j = 1; j <= 2; j++) {
        unsigned long long a = (unsigned long long)sqrt((double)n / j);
        for (unsigned long long i = a; i <= a + 1; i++) {
            unsigned long long q = cuda_mod_pow(i, (n - 1) / 2, n);
            if (q != 1 && q != n - 1) return false;
        }
    }
    
    return true;
}

__device__ bool cuda_mr_ge(long long potential_prime) {
    if (potential_prime == 2 || potential_prime == 3 ||
        potential_prime == 5 || potential_prime == 7) {
        return true;
    }
    if (potential_prime < 2 || !(potential_prime & 1)) {
        return false;
    }
    
    const unsigned long long n = (unsigned long long)potential_prime;
    
    unsigned long long t = n - 1;
    long long s = 0;
    while (!(t & 1)) {
        s++;
        t >>= 1;
    }
    
    const unsigned long long m = (unsigned long long)sqrt((double)n);
    const unsigned long long r = (unsigned long long)sqrt((double)n / 2);
    const unsigned long long prime[5] = {2, m + 1, m - 1, r + 1, r - 1};
    
    for (size_t x = 0; x < 5; x++) {
        const unsigned long long a = prime[x];
        unsigned long long b = cuda_mod_pow(a, t, n);
        
        for (long long y = 1; y <= s; y++) {
            const unsigned long long k = cuda_mod_mul(b, b, n);
            
            if (k == 1 && b != 1 && b != n - 1) {
                return false;
            }
            b = k;
        }
        
        if (b != 1) {
            return false;
        }
    }
    
    return true;
}

// CUDA kernel for parallel prime search
CUDA_GLOBAL_FN void cuda_prime_search_kernel(const long long* numbers, bool* results, int count, int method, int num_rounds) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < count) {
        long long potential_prime = numbers[idx];
        unsigned int seed = idx + 1; // Simple seed based on thread index
        
        switch (method) {
            case 0: // Naive
                results[idx] = cuda_naive_check(potential_prime);
                break;
            case 1: // Miller-Rabin
                results[idx] = cuda_miller_rabin(potential_prime, num_rounds, seed);
                break;
            case 2: // Fermat
                results[idx] = cuda_fermat(potential_prime, num_rounds, seed);
                break;
            case 3: // Gauss-Euler
                results[idx] = cuda_gauss_euler(potential_prime);
                break;
            case 4: // Miller-Rabin + Gauss-Euler
                results[idx] = cuda_mr_ge(potential_prime);
                break;
            default:
                results[idx] = false;
        }
    }
}

// Host wrapper function
void cuda_prime_search(const long long* numbers, bool* results, int count, int method, int num_rounds) {
#ifdef HAVE_CUDA
    long long* d_numbers;
    bool* d_results;
    
    // Allocate device memory
    cudaMalloc(&d_numbers, count * sizeof(long long));
    cudaMalloc(&d_results, count * sizeof(bool));
    
    // Copy data to device
    cudaMemcpy(d_numbers, numbers, count * sizeof(long long), cudaMemcpyHostToDevice);
    
    // Configure kernel launch parameters
    int threadsPerBlock = 256;
    int blocksPerGrid = (count + threadsPerBlock - 1) / threadsPerBlock;
    
    // Launch kernel
    cuda_prime_search_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_numbers, d_results, count, method, num_rounds);
    
    // Copy results back to host
    cudaMemcpy(results, d_results, count * sizeof(bool), cudaMemcpyDeviceToHost);
    
    // Free device memory
    cudaFree(d_numbers);
    cudaFree(d_results);
#else
    // Fallback CPU implementation when CUDA is not available
    printf("CUDA not available, using CPU fallback implementation...\n");
    for (int i = 0; i < count; i++) {
        long long potential_prime = numbers[i];
        unsigned int seed = i + 1;
        
        switch (method) {
            case 0: // Naive
                results[i] = cuda_naive_check(potential_prime);
                break;
            case 1: // Miller-Rabin
                results[i] = cuda_miller_rabin(potential_prime, num_rounds, seed);
                break;
            case 2: // Fermat
                results[i] = cuda_fermat(potential_prime, num_rounds, seed);
                break;
            case 3: // Gauss-Euler
                results[i] = cuda_gauss_euler(potential_prime);
                break;
            case 4: // Miller-Rabin + Gauss-Euler
                results[i] = cuda_mr_ge(potential_prime);
                break;
            default:
                results[i] = false;
        }
    }
#endif
}

void cuda_initialize() {
#ifdef HAVE_CUDA
    cudaSetDevice(0);
#endif
}

bool cuda_is_available() {
#ifdef HAVE_CUDA
    return true;
#else
    return false;
#endif
}