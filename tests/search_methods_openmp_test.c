#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#include "../src/search_methods_openmp.h"
#include "../src/utils.h"

void miller_rabin_test() {

    struct Config config = {8, 1, 100000000, 12};

    char filename[64];
    const time_t now = time(NULL);
    const struct tm *t = localtime(&now);

    snprintf(filename, sizeof(filename),
             "../results/miller_rabin_logs/miller_rabin_results_%04d-%02d-%02d_%02d-%02d-%02d.txt",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    FILE *fptr = fopen(filename, "w");

    if (fptr == NULL) {
        perror("Failed to open file");
        return;
    }

    const double start_overall = omp_get_wtime();
    long prime_count = 0;
    #pragma omp parallel for num_threads(config.num_threads)
    for (long long i = config.lower_range; i < config.max_range; ++i) {
        const double start = omp_get_wtime();
        const bool result = miller_rabin(i, &config);
        const double end = omp_get_wtime();;
        const double cpu_time_used = end - start;
        if (result) {
            #pragma omp critical
            // fprintf(fptr, "Potential prime: %lli took %f seconds to compute.\n", i, cpu_time_used);
            printf("Potential prime: %lli took %f seconds to compute.\n", i, cpu_time_used);
            prime_count += 1;
        }
    }
    const double final_overall = omp_get_wtime();
    const double overall_cpu_time = final_overall - start_overall;
    fclose(fptr);
    printf("Overall time taken for %li iterations: %f seconds. Total primes: %li. \n", config.max_range - config.lower_range, overall_cpu_time, prime_count);
}

void fermat_test() {

    struct Config config = {8, 1, 100000000, 20};

    long prime_count = 0;
    const double start_time = omp_get_wtime();

    #pragma omp parallel for num_threads(config.num_threads)
    for (long i = 0; i < config.max_range; ++i) {
        const double start_time_p = omp_get_wtime();
        const bool prime_result = fermat(i, &config);
        const double end_time_p = omp_get_wtime();
        const double cpu_time_used = end_time_p - start_time_p;

        if (prime_result) {
            printf("Potential prime: %li took %f seconds to compute.\n", i, cpu_time_used);
            ++prime_count;
        }

    }
    const double final_time = omp_get_wtime();
    const double overall_cpu_time = final_time - start_time;
    printf("Overall time taken for %li iterations: %f seconds. Total primes: %li. \n", config.max_range - config.lower_range, overall_cpu_time, prime_count);
}

void fermat_test_predefined_arrays() {

    struct Config config = {3, 1, 1, 20};

    #pragma omp parallel for num_threads(config.num_threads)
    for (size_t i = 0; i < TEST_ARRAY_SIZES; ++i) {
        const double start_time_p = omp_get_wtime();
        const bool prime_result = fermat(TEST_PRIMES[i], &config);
        const double end_time_p = omp_get_wtime();
        const double cpu_time_used = end_time_p - start_time_p;

        const bool composite_result = fermat(TEST_COMPOSITE[i], &config);

        if (!prime_result) {
            printf("Failed prime number check for %d\n", TEST_PRIMES[i]);
        } else {
            printf("Potential prime: %lli took %f seconds to compute.\n", i, cpu_time_used);
        }

        if (composite_result) {
            printf("Failed composite number check for %d\n", TEST_COMPOSITE[i]);
        };
    }
}

void gauss_euler_test_predefined_arrays() {

    struct Config config = {3, 1, 1, 20}; // values dont matter

    #pragma omp parallel for num_threads(config.num_threads)
    for (size_t i = 0; i < TEST_ARRAY_SIZES; ++i) {
        const bool prime_result = gauss_euler(TEST_PRIMES[i]);
        const bool composite_result = gauss_euler(TEST_COMPOSITE[i]);

        if (!prime_result) {
            printf("Failed prime number check for %d\n", TEST_PRIMES[i]);
        }
        if (composite_result) {
            printf("Failed composite number check for %d\n", TEST_COMPOSITE[i]);
        };
    }
}

void gauss_euler_test() {

    struct Config config = {8, 1000000000000000001, 1000000000000199999, 20};

    long prime_count = 0;
    const double start_time = omp_get_wtime();
    #pragma omp parallel for num_threads(config.num_threads)
    for (long long i = config.lower_range; i < config.max_range; ++i) {
        const double start_time_p = omp_get_wtime();
        const bool prime_result = gauss_euler(i);
        const double end_time_p = omp_get_wtime();
        const double cpu_time_used = end_time_p - start_time_p;

        if (prime_result) {
            printf("Potential prime: %lli took %f seconds to compute.\n", i, cpu_time_used);
            ++prime_count;
        }
        // if (!prime_result) {
        //     printf("Failed prime number check for %lli\n", i);
        // }

    }
    const double final_time = omp_get_wtime();
    const double overall_cpu_time = final_time - start_time;
    printf("Overall time taken for %li iterations: %f seconds. Total primes: %li. \n", config.max_range - config.lower_range, overall_cpu_time, prime_count);
}

void naive_check_test() {

    struct Config config = {3, 1, 1, 1}; // values dont matter

    #pragma omp parallel for num_threads(config.num_threads)
    for (size_t i = 0; i < TEST_ARRAY_SIZES; ++i) {
        const bool prime_result = naive_check(TEST_PRIMES[i], &config);
        const bool composite_result = naive_check(TEST_COMPOSITE[i], &config);

        if (!prime_result) {
            printf("Failed prime number check for %d\n", TEST_PRIMES[i]);
        };

        if (composite_result) {
            printf("Failed composite number check for %d\n", TEST_COMPOSITE[i]);
        };
    }
}

void mr_ge_test_predefined_arrays() {
    struct Config config = {3, 1, 1, 20}; // values dont matter

    #pragma omp parallel for num_threads(config.num_threads)
    for (size_t i = 0; i < TEST_ARRAY_SIZES; ++i) {
        const bool prime_result = mr_ge(TEST_PRIMES[i]);
        const bool composite_result = mr_ge(TEST_COMPOSITE[i]);

        if (!prime_result) {
            printf("Failed prime number check for %d\n", TEST_PRIMES[i]);
        }
        if (composite_result) {
            printf("Failed composite number check for %d\n", TEST_COMPOSITE[i]);
        };
    }
}

void mr_ge_test() {

    struct Config config = {8, 1000000000000000001, 1000000000000199999, 20};

    long prime_count = 0;
    const double start_time = omp_get_wtime();
    #pragma omp parallel for num_threads(config.num_threads)
    for (long long i = config.lower_range; i < config.max_range; ++i) {
        const double start_time_p = omp_get_wtime();
        const bool prime_result = gauss_euler(i);
        const double end_time_p = omp_get_wtime();
        const double cpu_time_used = end_time_p - start_time_p;

        if (prime_result) {
            printf("Potential prime: %lli took %f seconds to compute.\n", i, cpu_time_used);
            ++prime_count;
        }

    }
    const double final_time = omp_get_wtime();
    const double overall_cpu_time = final_time - start_time;
    printf("Overall time taken for %li iterations: %f seconds. Total primes: %li. \n", config.max_range - config.lower_range, overall_cpu_time, prime_count);
}

int main() {
    // fermat_test();
    // naive_check_test();
    // miller_rabin_test();
    // gauss_euler_test_predefined_arrays();
    // gauss_euler_test();
    // mr_ge_test_predefined_arrays();
    mr_ge_test();
}