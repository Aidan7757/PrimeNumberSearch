#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "models.h"
#include "utils.h"

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

/**
 * Fermat primality test, probabilistic primality test.
 * Link: https://en.wikipedia.org/wiki/Fermat_primality_test
 *
 * @param potential_prime the number to check primality of.
 * @param config struct config.
 * @return true or false if the number is prime.
 */
bool fermat(const long long potential_prime, const struct Config* config) {
    if (potential_prime == 1) return false;
    if (potential_prime == 2 || potential_prime == 3) return true;
    if (potential_prime % 2 == 0) return false;

    for (size_t i = 2; i < config->num_rounds; ++i) {
        unsigned int seed = (unsigned int)(time(NULL) + i + omp_get_thread_num());
        const unsigned long long a = (rand_r(&seed) % (potential_prime - 3)) + 2;
        const unsigned long long result = mod_pow(a, potential_prime - 1, potential_prime);
        if (result != 1) return false;
    }
    return true;
}

/**
 * Gauss Euler primality test. Link: https://arxiv.org/pdf/2311.07048
 *
 * @param potential_prime prime to check

 * @return
 */
bool gauss_euler(const long long potential_prime) {
    if (potential_prime == 2) return true;
    if (!(potential_prime & 1) || potential_prime < 2) return false;

    const unsigned long long n = (unsigned long long)potential_prime;

    // Euler criterion for 2
    unsigned long long t = mod_pow(2, (n - 1) / 2, n);
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
            unsigned long long q = mod_pow(i, (n - 1) / 2, n);
            if (q != 1 && q != n - 1) return false;
        }
    }

    // Find first prime p1 ≡ 5 (mod 8) where n is not a quadratic residue
    unsigned long long p1;
    for (p1 = 5; ; p1 += 8) {
        // Check if p1 is prime
        unsigned long long i;
        for (i = 3; i * i <= p1; i += 2) {
            if (p1 % i == 0) break;
        }

        // Check if n is a quadratic non-residue mod p1
        unsigned long long j;
        for (j = 1; j <= (p1 - 1) / 2; j++) {
            if (i * i <= p1 || n % p1 == 0 || n % p1 == (j * j) % p1) break;
        }

        if (i * i > p1 && j > (p1 - 1) / 2) break;
    }

    if (mod_pow(p1, (n - 1) / 2, n) != n - 1) {
        return false;
    }

    // Find first prime p2 ≡ 1 (mod 8) where n is not a quadratic residue
    unsigned long long p2;
    for (p2 = 17; ; p2 += 8) {
        // Check if p2 is prime
        unsigned long long i;
        for (i = 3; i * i <= p2; i += 2) {
            if (p2 % i == 0) break;
        }

        // Check if n is a quadratic non-residue mod p2
        unsigned long long j;
        for (j = 1; j <= (p2 - 1) / 2; j++) {
            if (i * i <= p2 || n % p2 == 0 || n % p2 == (j * j) % p2) break;
        }

        if (i * i > p2 && j > (p2 - 1) / 2) break;
    }

    if (mod_pow(p2, (n - 1) / 2, n) != n - 1) {
        return false;
    }

    return true;
}