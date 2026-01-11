#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "models.h"


int factor_out_twos(const long potential_prime, long* d) {
    long value = potential_prime - 1;
    int s = 0;

    while (value % 2 == 0) {
        value /= 2;
        s += 1;
    }
    *d = value;
    return s;
}

/**
 * Naive check for if a number is prime. Non probablistic.
 * @param potential_prime long: potential prime number to be checked.
 * @param config config
 * @return true or false on if the number is detected to be prime or not.
 */
bool naive_check(const long potential_prime, struct Config* config) {

    if (potential_prime == 1) return true;
    if (potential_prime == 2) return true;

    // go through and mod with each number until potential prime - 1, if mod == 0 then non prime

    bool overall_result = true;

    #pragma omp parallel num_threads(config->num_threads)
    #pragma omp parallel for
    for (size_t i = 2; i < potential_prime - 1; ++i) {
        if (potential_prime % i == 0) overall_result = false;
    }
    return overall_result;
}

/**
 * Miller-Rabin primality test, probabilistic primality test. Link: https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
 *
 * @param potential_prime prime to check.
 * @param config settings.
 * @return true or false on if likely to be prime or not.
 */
bool miller_rabin(const long potential_prime, const struct Config* config) {
    if (potential_prime == 1) return true;
    if (potential_prime == 2) return true;

    long d;
    const int s = factor_out_twos(potential_prime, &d);
    const long range_size = (potential_prime - 2) - 2 + 1;
    bool overall_result = true;

    #pragma omp parallel num_threads(config->num_threads)
    #pragma omp parallel for
    for (size_t i = 0; i < config->num_rounds; ++i) {

        unsigned int seed = omp_get_thread_num() * time(NULL); // random value using thread_num and time
        const long a = rand_r(&seed) % range_size + 2;
        long x = (int) pow((double) a, (double) d) % potential_prime;
        long y = 0;

        for (size_t j = 0; j < s; ++j) {
            y = (int) pow((double) x, 2.0) % potential_prime;
            if (y == 1 && x != 1 && x != potential_prime - 1) overall_result = false;
            x = y;
        }
        if (y != 1) overall_result = false;
    }

    return overall_result;
}
