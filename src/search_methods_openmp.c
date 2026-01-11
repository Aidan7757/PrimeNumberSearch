#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "models.h"

// Modular exponentiation: computes (base^exp) % mod
unsigned long long mod_pow(unsigned long long base, unsigned long long exp, unsigned long long mod) {
    unsigned long long res = 1;
    base %= mod;

    while (exp > 0) {
        if (exp & 1) {
            res = (res * base) % mod;
        }
        base = (base * base) % mod;
        exp >>= 1;
    }
    return res;
}

long factor_out_twos(const unsigned long long potential_prime, unsigned long long* d) {
    unsigned long long value = potential_prime - 1;
    long s = 0;

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
bool naive_check(const long long potential_prime, struct Config* config) {
    if (potential_prime <= 1) return false;
    if (potential_prime == 2) return true;
    if (potential_prime % 2 == 0) return false;

    bool overall_result = true;

    #pragma omp parallel num_threads(config->num_threads)
    #pragma omp parallel for
    for (size_t i = 2; i < potential_prime - 1; ++i) {
        if (potential_prime % i == 0) overall_result = false;
    }
    return overall_result;
}

/**
 * Miller-Rabin primality test, probabilistic primality test.
 * Link: https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
 *
 * @param potential_prime prime to check.
 * @param config settings.
 * @return true or false on if likely to be prime or not.
 */
bool miller_rabin(const long long potential_prime, const struct Config* config) {
    // Handle base cases
    if (potential_prime <= 1) return false;
    if (potential_prime == 2 || potential_prime == 3) return true;
    if (potential_prime % 2 == 0) return false;

    // Factor out powers of 2: potential_prime - 1 = 2^s * d
    unsigned long long d;
    const long s = factor_out_twos(potential_prime, &d);

    // Perform k rounds of testing
    for (size_t round = 0; round < config->num_rounds; ++round) {
        // Pick random witness a in range [2, potential_prime - 2]
        unsigned int seed = (unsigned int)(time(NULL) + round + omp_get_thread_num());
        const unsigned long long a = (rand_r(&seed) % (potential_prime - 3)) + 2;

        // Compute x = a^d mod potential_prime
        unsigned long long x = mod_pow(a, d, potential_prime);

        if (x == 1 || x == potential_prime - 1) {
            continue;
        }

        // Square x repeatedly (s-1) times
        bool composite = true;
        for (long j = 0; j < s - 1; ++j) {
            x = mod_pow(x, 2, potential_prime);

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