#include <stdbool.h>
#include <stdio.h>
#include <omp.h>

#include "../src/search_methods_openmp.h"

void general_prime_cpu_test(bool (*func) (long long prime_to_check, struct Config* config)) {
    struct Config config = {8, 1000000000000000000,
        1000000000010000000, 8};

    long prime_count = 0;
    const double start_time = omp_get_wtime();
    #pragma omp parallel for num_threads(config.num_threads)
    for (long long i = config.lower_range; i < config.max_range; ++i) {
        // figure out how to have only one of these print
        // statements for config be printed
        const bool prime_result = func(i, &config);
        if (prime_result) {
            ++prime_count;
        }
    }
    const double final_time = omp_get_wtime();
    const double overall_cpu_time = final_time - start_time;
    printf("Overall time taken for %lli iterations: %f seconds. "
           "Total primes: %li. \n", config.max_range - config.lower_range, overall_cpu_time, prime_count);
}

int main() {
    typedef bool (*f) (long long prime_to_check, struct Config* config);
    const f functions[5] = {&naive_check, &miller_rabin, &fermat, &gauss_euler, &mr_ge};
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        general_prime_cpu_test(functions[i]);
    }
}