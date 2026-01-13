#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "src/models.h"
#include "src/search_methods_cuda.h"

void print_usage(const char* program_name) {
    printf("Usage: %s <method> <start> <end> <rounds>\n", program_name);
    printf("Methods:\n");
    printf("  0 - Naive check\n");
    printf("  1 - Miller-Rabin\n");
    printf("  2 - Fermat\n");
    printf("  3 - Gauss-Euler\n");
    printf("  4 - Miller-Rabin + Gauss-Euler\n");
    printf("Example: %s 1 1000000 1000100 10\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        print_usage(argv[0]);
        return 1;
    }
    
    int method = atoi(argv[1]);
    long long start = atoll(argv[2]);
    long long end = atoll(argv[3]);
    int rounds = atoi(argv[4]);
    
    if (method < 0 || method > 4) {
        printf("Error: Invalid method. Must be 0-4.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (start >= end) {
        printf("Error: start must be less than end.\n");
        return 1;
    }
    
    // Initialize CUDA
    cuda_initialize();
    
    // Check CUDA availability
    if (!cuda_is_available()) {
        printf("Note: CUDA not available, using CPU fallback implementation.\n");
    }
    
    // Calculate range and allocate arrays
    long long count = end - start;
    long long* numbers = (long long*)malloc(count * sizeof(long long));
    bool* results = (bool*)malloc(count * sizeof(bool));
    
    if (!numbers || !results) {
        printf("Error: Memory allocation failed.\n");
        free(numbers);
        free(results);
        return 1;
    }
    
    // Fill numbers array
    for (long long i = 0; i < count; i++) {
        numbers[i] = start + i;
    }
    
    printf("Searching for primes in range [%lld, %lld) using method %d with %d rounds...\n", 
           start, end, method, rounds);
    
    // Start timing
    clock_t start_time = clock();
    
    // Run CUDA prime search
    cuda_prime_search(numbers, results, count, method, rounds);
    
    // End timing
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Count and display results
    long long prime_count = 0;
    for (long long i = 0; i < count; i++) {
        if (results[i]) {
            prime_count++;
            if (prime_count <= 20) { // Show first 20 primes
                printf("Prime found: %lld\n", numbers[i]);
            }
        }
    }
    
    printf("\nSummary:\n");
    printf("  Numbers checked: %lld\n", count);
    printf("  Primes found: %lld\n", prime_count);
    printf("  Time elapsed: %.4f seconds\n", elapsed_time);
    printf("  Throughput: %.0f numbers/second\n", count / elapsed_time);
    
    // Free memory
    free(numbers);
    free(results);
    
    return 0;
}