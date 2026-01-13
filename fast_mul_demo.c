#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "src/models.h"
#include "src/search_methods_cuda.h"

// Forward declaration for fast multiplication function
void cuda_fast_mod_multiply(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count);
void cuda_schonhage_strassen_multiply(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count);

void test_multiplication_performance() {
    printf("Testing multiplication performance with different methods...\n\n");
    
    // Test with different number sizes
    uint64_t test_sizes[] = {
        1000ULL,           // Small
        1000000ULL,        // Medium  
        1000000000ULL,     // Large
        1000000000000ULL,  // Very Large
        1000000000000000ULL // Extremely Large
    };
    
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    const char* size_names[] = {"Small", "Medium", "Large", "Very Large", "Extremely Large"};
    
    printf("Number Size     | Traditional (ns) | FFT-based (ns) | Schönhage-Strassen (ns)\n");
    printf("---------------|------------------|----------------|-------------------------\n");
    
    for (int i = 0; i < num_sizes; i++) {
        uint64_t a = test_sizes[i];
        uint64_t b = test_sizes[i] + 12345; // Different value
        uint64_t mod = test_sizes[i] * 2 + 1; // Ensure it's reasonably sized
        
        // Test traditional multiplication
        clock_t start = clock();
        for (int j = 0; j < 1000; j++) {
            volatile uint64_t temp_result = (a * b) % mod; // Traditional for comparison
        }
        clock_t end = clock();
        double traditional_time = ((double)(end - start) / 1000.0) * 1000000; // nanoseconds
        
        // Test FFT-based multiplication
        start = clock();
        for (int j = 0; j < 1000; j++) {
            uint64_t result;
            cuda_fast_mod_multiply(&a, &b, &mod, &result, 1);
        }
        end = clock();
        double fft_time = ((double)(end - start) / 1000.0) * 1000000; // nanoseconds
        
        // Test Schönhage-Strassen multiplication
        start = clock();
        for (int j = 0; j < 1000; j++) {
            uint64_t result;
            cuda_schonhage_strassen_multiply(&a, &b, &mod, &result, 1);
        }
        end = clock();
        double ss_time = ((double)(end - start) / 1000.0) * 1000000; // nanoseconds
        
        printf("%-14s | %16.0f | %14.0f | %23.0f\n", 
               size_names[i], traditional_time, fft_time, ss_time);
    }
    printf("\n");
}

void test_prime_search_with_fast_mul() {
    printf("Testing prime search with fast multiplication...\n\n");
    
    // Initialize CUDA
    cuda_initialize();
    
    // Test range with large numbers
    const long long start = 1000000000000LL; // 1 trillion
    const long long end = 1000000000100LL;    // 100 numbers
    const int rounds = 5;
    const int count = end - start;
    
    // Allocate arrays
    long long* numbers = (long long*)malloc(count * sizeof(long long));
    bool* results = (bool*)malloc(count * sizeof(bool));
    
    if (!numbers || !results) {
        printf("Memory allocation failed!\n");
        free(numbers);
        free(results);
        return;
    }
    
    // Fill test numbers
    for (int i = 0; i < count; i++) {
        numbers[i] = start + i;
    }
    
    printf("Testing Miller-Rabin with fast multiplication on large numbers [%lld, %lld)\n", start, end);
    
    clock_t start_time = clock();
    
    // Run prime search with fast multiplication
    cuda_prime_search(numbers, results, count, 1, rounds); // Method 1 = Miller-Rabin
    
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Count primes found
    int prime_count = 0;
    for (int i = 0; i < count; i++) {
        if (results[i]) {
            prime_count++;
            if (prime_count <= 5) {
                printf("Prime found: %lld\n", numbers[i]);
            }
        }
    }
    
    printf("\nPerformance Summary:\n");
    printf("  Numbers tested: %d\n", count);
    printf("  Primes found: %d\n", prime_count);
    printf("  Time elapsed: %.4f seconds\n", elapsed_time);
    printf("  Throughput: %.0f numbers/second\n", count / elapsed_time);
    printf("  Fast multiplication method: Schönhage-Strassen + FFT\n");
    
    // Free memory
    free(numbers);
    free(results);
}

void demonstrate_algorithm() {
    printf("=== Fast Multiplication for Prime Number Search ===\n\n");
    
    printf("Algorithm Overview:\n");
    printf("1. For small numbers (< 2^16): Traditional binary multiplication\n");
    printf("2. For medium numbers (2^16 to 2^20): FFT-based convolution\n");
    printf("3. For large numbers (> 2^20): Schönhage-Strassen algorithm\n\n");
    
    printf("Schönhage-Strassen Algorithm Steps:\n");
    printf("• Split numbers into polynomial coefficients\n");
    printf("• Apply weighting with powers of θ\n");
    printf("• Shuffle coefficients (bit-reversal)\n");
    printf("• Evaluate using FFT (complex multiplication)\n");
    printf("• Pointwise multiplication in frequency domain\n");
    printf("• Inverse FFT and unweighting\n");
    printf("• Carry propagation and normalization\n");
    printf("• Modulo reduction\n\n");
    
    test_multiplication_performance();
    test_prime_search_with_fast_mul();
    
    printf("\n=== Integration with Prime Testing ===\n");
    printf("The fast multiplication is integrated into:\n");
    printf("• Miller-Rabin: Modular exponentiation (a^d mod n)\n");
    printf("• Fermat: Modular exponentiation (a^(n-1) mod n)\n");
    printf("• Gauss-Euler: Multiple modular multiplications\n");
    printf("• MR-GE: Combined operations\n");
}

int main(int argc, char* argv[]) {
    demonstrate_algorithm();
    return 0;
}