#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "search_methods_cuda.h"



// Forward declarations for CPU implementations
static bool cpu_naive_check(long long potential_prime);
static bool cpu_miller_rabin(long long potential_prime, int num_rounds);
static bool cpu_fermat(long long potential_prime, int num_rounds);
static bool cpu_gauss_euler(long long potential_prime);
static bool cpu_mr_ge(long long potential_prime);

// Host wrapper functions for when CUDA is not available
void cuda_initialize() {
    // No initialization needed for CPU fallback
}

bool cuda_is_available() {
    return false; // CUDA is not available
}

// Additional host functions for fast multiplication

void cuda_prime_search(const long long* numbers, bool* results, int count, int method, int num_rounds) {
    printf("CUDA not available, using CPU fallback implementation...\n");
    
    // Simple CPU implementation of the primality tests
    for (int i = 0; i < count; i++) {
        long long potential_prime = numbers[i];
        
        switch (method) {
            case 0: // Naive
                results[i] = cpu_naive_check(potential_prime);
                break;
            case 1: // Miller-Rabin
                results[i] = cpu_miller_rabin(potential_prime, num_rounds);
                break;
            case 2: // Fermat
                results[i] = cpu_fermat(potential_prime, num_rounds);
                break;
            case 3: // Gauss-Euler
                results[i] = cpu_gauss_euler(potential_prime);
                break;
            case 4: // Miller-Rabin + Gauss-Euler
                results[i] = cpu_mr_ge(potential_prime);
                break;
            default:
                results[i] = false;
        }
    }
}

// CPU implementations of the primality tests
static bool cpu_naive_check(long long potential_prime) {
    if (potential_prime <= 1) return false;
    if (potential_prime == 2) return true;
    if (potential_prime % 2 == 0) return false;
    
    for (long long i = 3; i * i <= potential_prime; i += 2) {
        if (potential_prime % i == 0) return false;
    }
    return true;
}

static bool cpu_miller_rabin(long long potential_prime, int num_rounds) {
    if (potential_prime <= 1) return false;
    if (potential_prime == 2 || potential_prime == 3) return true;
    if (potential_prime % 2 == 0) return false;
    
    // Write n-1 as d * 2^s
    unsigned long long d = potential_prime - 1;
    long long s = 0;
    while (d % 2 == 0) {
        d /= 2;
        s++;
    }
    
    // Miller-Rabin test
    for (int round = 0; round < num_rounds; round++) {
        // Use a simple deterministic set of bases for small numbers
        unsigned long long a;
        if (potential_prime < 1373653) {
            a = (round == 0) ? 2 : 3;
        } else if (potential_prime < 25326001) {
            a = (round == 0) ? 2 : (round == 1) ? 3 : 5;
        } else {
            a = 2 + round; // Simple fallback
        }
        
        if (a >= potential_prime - 2) continue;
        
        // Compute a^d mod n
        unsigned long long x = 1;
        unsigned long long base = a % potential_prime;
        unsigned long long exp = d;
        
        while (exp > 0) {
            if (exp & 1) {
                __uint128_t temp = (__uint128_t)x * base;
                x = temp % potential_prime;
            }
            __uint128_t temp = (__uint128_t)base * base;
            base = temp % potential_prime;
            exp >>= 1;
        }
        
        if (x == 1 || x == potential_prime - 1) {
            continue;
        }
        
        bool composite = true;
        for (long long j = 0; j < s - 1; j++) {
            __uint128_t temp = (__uint128_t)x * x;
            x = temp % potential_prime;
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

static bool cpu_fermat(long long potential_prime, int num_rounds) {
    if (potential_prime == 1) return false;
    if (potential_prime == 2 || potential_prime == 3) return true;
    if (potential_prime % 2 == 0) return false;
    
    for (int i = 2; i < num_rounds && i < potential_prime - 1; ++i) {
        unsigned long long result = 1;
        unsigned long long base = i % potential_prime;
        unsigned long long exp = potential_prime - 1;
        
        while (exp > 0) {
            if (exp & 1) {
                __uint128_t temp = (__uint128_t)result * base;
                result = temp % potential_prime;
            }
            __uint128_t temp = (__uint128_t)base * base;
            base = temp % potential_prime;
            exp >>= 1;
        }
        
        if (result != 1) return false;
    }
    return true;
}

static bool cpu_gauss_euler(long long potential_prime) {
    // Simplified Gauss-Euler test - fallback to Miller-Rabin
    return cpu_miller_rabin(potential_prime, 5);
}

static bool cpu_mr_ge(long long potential_prime) {
    // Simplified MR-GE test - fallback to Miller-Rabin
    return cpu_miller_rabin(potential_prime, 10);
}