#include <stdbool.h>
#include <stdio.h>
#include <omp.h>
#include "../src/search_methods_openmp.h"
#include "../src/models.h"
#include "../src/bn.h"

void general_prime_cpu_test(bool (*func)(BigNum* prime_to_check, Config* config),
                            Config* config) {
    long long prime_count = 0;
    const double start_time = omp_get_wtime();

    if (config->verbose) {
        printf("\n========================================\n");
        printf("Starting Prime Search\n");
        config_print(config);
        printf("========================================\n\n");
    }

    // Calculate range size for iteration
    BigNum range_size;
    bn_sub(&range_size, &config->max_range, &config->lower_range);
    __int128_t iterations = bn_to_int128(&range_size);

    #pragma omp parallel num_threads(config->num_threads) reduction(+:prime_count)
    {
        BigNum current;
        bn_init(&current);

        #pragma omp for schedule(dynamic, 1000)
        for (__int128_t i = 0; i < iterations; ++i) {
            // current = lower_range + i
            BigNum offset;
            bn_from_int128(&offset, i);
            bn_add(&current, &config->lower_range, &offset);

            const bool prime_result = func(&current, config);
            if (prime_result) {
                prime_count++;
            }
        }
    }

    const double final_time = omp_get_wtime();
    const double overall_cpu_time = final_time - start_time;

    printf("Method: %s\n", config_get_mul_method_name(config->mul_method));
    printf("Time:   %.6f seconds\n", overall_cpu_time);
    printf("Range:  %lld iterations\n", (long long)iterations);
    printf("Primes: %lld found\n", prime_count);
    printf("Speed:  %.2f checks/sec\n\n",
           (double)iterations / overall_cpu_time);
}

int main() {
    typedef bool (*f) (BigNum*, Config*);
    f functions[] = {
        (f)&miller_rabin,
        (f)&fermat,
        (f)&gauss_euler,
        (f)&mr_ge
    };
    const char* func_names[] = {"Miller-Rabin", "Fermat", "Gauss-Euler", "MR-GE"};

    // Test configurations - all using 2048 bits
    const int BIT_SIZE = 2048;
    Config configs[3];

    // Generate a single random starting point
    BigNum base_range;
    bn_random(&base_range, BIT_SIZE);

    BigNum offset;
    bn_from_int128(&offset, 1000000);

    // Config 0: BigNum Standard
    config_init(&configs[0]);
    bn_copy(&configs[0].lower_range, &base_range);
    bn_add(&configs[0].max_range, &base_range, &offset);
    configs[0].mul_method = MUL_BIGNUM_STANDARD;
    configs[0].use_bignum = true;
    configs[0].bignum_bit_size = BIT_SIZE;

    // Config 1: BigNum Karatsuba
    config_init(&configs[1]);
    bn_copy(&configs[1].lower_range, &base_range);
    bn_add(&configs[1].max_range, &base_range, &offset);
    configs[1].mul_method = MUL_BIGNUM_KARATSUBA;
    configs[1].use_bignum = true;
    configs[1].bignum_bit_size = BIT_SIZE;

    // Config 2: BigNum Schönhage-Strassen
    config_init(&configs[2]);
    bn_copy(&configs[2].lower_range, &base_range);
    bn_add(&configs[2].max_range, &base_range, &offset);
    configs[2].mul_method = MUL_BIGNUM_SS;
    configs[2].use_bignum = true;
    configs[2].bignum_bit_size = BIT_SIZE;

    printf("========================================\n");
    printf("  PRIME SEARCH BENCHMARK\n");
    printf("  Bit Size: %d\n", BIT_SIZE);
    printf("========================================\n");

    // Test each primality function with each config
    for (size_t func_idx = 0; func_idx < sizeof(functions) / sizeof(functions[0]); ++func_idx) {
        printf("\n\n=== Testing: %s ===\n", func_names[func_idx]);

        for (size_t cfg_idx = 0; cfg_idx < 3; ++cfg_idx) {
            general_prime_cpu_test(functions[func_idx], &configs[cfg_idx]);
        }
    }

    printf("\n========================================\n");

    return 0;
}