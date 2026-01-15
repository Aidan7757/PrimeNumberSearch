#include <stdbool.h>
#include <stdio.h>
#include <omp.h>
#include "../src/search_methods_openmp.h"
#include "../src/models.h"

void general_prime_cpu_test(bool (*func)(__int128_t prime_to_check, Config* config),
                            Config* config) {
    long long prime_count = 0;
    const double start_time = omp_get_wtime();

    if (config->verbose) {
        printf("\n========================================\n");
        printf("Starting Prime Search\n");
        config_print(config);
        printf("========================================\n\n");
    }

    #pragma omp parallel for num_threads(config->num_threads) \
            reduction(+:prime_count) schedule(dynamic, 1000)
    for (__int128_t i = config->lower_range; i < config->max_range; ++i) {
        const bool prime_result = func(i, config);
        if (prime_result) {
            prime_count++;
        }
    }

    const double final_time = omp_get_wtime();
    const double overall_cpu_time = final_time - start_time;

    printf("Method: %s\n", config_get_mul_method_name(config->mul_method));
    printf("Time:   %.6f seconds\n", overall_cpu_time);
    printf("Range:  %lld iterations\n",
           (long long)(config->max_range - config->lower_range));
    printf("Primes: %lld found\n", prime_count);
    printf("Speed:  %.2f checks/sec\n\n",
           (double)(config->max_range - config->lower_range) / overall_cpu_time);
}

int main() {
    typedef bool (*f) (__int128_t, Config*);
    f functions[] = {
        (f)&miller_rabin,
        (f)&fermat,
        (f)&gauss_euler,
        (f)&mr_ge
    };
    const char* func_names[] = {"Miller-Rabin", "Fermat", "Gauss-Euler", "MR-GE"};

    // Test configurations
    Config configs[5];

    // Config 0: Native __int128_t (baseline)
    config_init(&configs[0]);
    configs[0].mul_method = MUL_NATIVE_INT128;
    configs[0].use_bignum = false;

    // Config 1: BigNum Standard
    config_init(&configs[1]);
    configs[1].mul_method = MUL_BIGNUM_STANDARD;
    configs[1].use_bignum = true;

    // Config 2: BigNum Karatsuba
    config_init(&configs[2]);
    configs[2].mul_method = MUL_BIGNUM_KARATSUBA;
    configs[2].use_bignum = true;

    // Config 3: BigNum Schönhage-Strassen
    config_init(&configs[3]);
    configs[3].mul_method = MUL_BIGNUM_SS;
    configs[3].use_bignum = true;

    // Config 4: Auto-select (with verification)
    config_init(&configs[4]);
    configs[4].mul_method = MUL_AUTO;
    configs[4].use_bignum = true;
    configs[4].verify_results = true;  // Verify against native

    printf("========================================\n");
    printf("  PRIME SEARCH BENCHMARK\n");
    printf("========================================\n");

    // Test each primality function with each config
    for (size_t func_idx = 0; func_idx < sizeof(functions) / sizeof(functions[0]); ++func_idx) {
        printf("\n\n=== Testing: %s ===\n", func_names[func_idx]);

        for (size_t cfg_idx = 0; cfg_idx < 5; ++cfg_idx) {
            general_prime_cpu_test(functions[func_idx], &configs[cfg_idx]);
        }
    }

    printf("\n========================================\n");

    return 0;
}