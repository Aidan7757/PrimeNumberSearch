#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "models.h"
#include "bn.h"

/**
 * Naive check for if a number is prime. Non probablistic.
 * @param potential_prime BigNum: potential prime number to be checked.
 * @param config config
 * @return true or false on if the number is detected to be prime or not.
 */
bool naive_check(BigNum* potential_prime, Config* config) {
    BigNum zero, one, two;
    bn_init(&zero);
    bn_init(&one); one.words[0] = 1;
    bn_init(&two); two.words[0] = 2;

    if (bn_compare(potential_prime, &one) <= 0) return false;
    if (bn_compare(potential_prime, &two) == 0) return true;
    if (bn_is_even(potential_prime)) return false;

    bool overall_result = true;

    BigNum i, result;
    bn_copy(&i, &two);

    BigNum n_minus_1;
    bn_sub(&n_minus_1, potential_prime, &one);

    while (bn_compare(&i, &n_minus_1) < 0) {
        bn_mod_mul(&result, potential_prime, &i, potential_prime, config->mul_method);
        if (bn_is_zero(&result)) {
            overall_result = false;
            break;
        }
        bn_add(&i, &i, &one);
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
bool miller_rabin(BigNum* potential_prime, Config* config) {
    // Handle base cases
    BigNum zero, one, two, three;
    bn_init(&zero);
    bn_init(&one); one.words[0] = 1;
    bn_init(&two); two.words[0] = 2;
    bn_init(&three); three.words[0] = 3;

    if (bn_compare(potential_prime, &one) <= 0) return false;
    if (bn_compare(potential_prime, &two) == 0 ||
        bn_compare(potential_prime, &three) == 0) return true;
    if (bn_is_even(potential_prime)) return false;

    // Factor out powers of 2: potential_prime - 1 = 2^s * d
    BigNum n_minus_1, d;
    bn_sub(&n_minus_1, potential_prime, &one);
    bn_copy(&d, &n_minus_1);

    int s = 0;
    while (bn_is_even(&d)) {
        bn_shift_right(&d, 1);
        s++;
    }

    // Determine multiplication method
    int method = config->mul_method;
    if (method == MUL_AUTO) {
        method = bn_auto_select_mul_method(potential_prime, potential_prime);
    }

    // Perform k rounds of testing
    for (size_t round = 0; round < config->num_rounds; ++round) {
        // Pick random witness a in range [2, potential_prime - 2]
        BigNum a, n_minus_2;
        bn_sub(&n_minus_2, potential_prime, &two);
        bn_random_range(&a, &two, &n_minus_2);

        // Compute x = a^d mod potential_prime
        BigNum x;
        bn_mod_pow(&x, &a, &d, potential_prime, method);

        if (bn_compare(&x, &one) == 0 ||
            bn_compare(&x, &n_minus_1) == 0) {
            continue;
        }

        // Square x repeatedly (s-1) times
        bool composite = true;
        for (int j = 0; j < s - 1; ++j) {
            bn_mod_mul(&x, &x, &x, potential_prime, method);

            if (bn_compare(&x, &n_minus_1) == 0) {
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
 * @param config config.
 * @return true or false if the number is prime.
 */
bool fermat(BigNum* potential_prime, Config* config) {
    BigNum one, two, three;
    bn_init(&one); one.words[0] = 1;
    bn_init(&two); two.words[0] = 2;
    bn_init(&three); three.words[0] = 3;

    if (bn_compare(potential_prime, &one) == 0) return false;
    if (bn_compare(potential_prime, &two) == 0 ||
        bn_compare(potential_prime, &three) == 0) return true;
    if (bn_is_even(potential_prime)) return false;

    // Determine multiplication method
    int method = config->mul_method;
    if (method == MUL_AUTO) {
        method = bn_auto_select_mul_method(potential_prime, potential_prime);
    }

    BigNum n_minus_1, n_minus_3;
    bn_sub(&n_minus_1, potential_prime, &one);
    bn_sub(&n_minus_3, potential_prime, &three);

    for (size_t i = 0; i < config->num_rounds; ++i) {
        // Pick random a in range [2, potential_prime - 2]
        BigNum a;
        bn_random_range(&a, &two, &n_minus_3);
        bn_add(&a, &a, &two);  // Ensure a >= 2

        // Compute result = a^(n-1) mod n
        BigNum result;
        bn_mod_pow(&result, &a, &n_minus_1, potential_prime, method);

        if (bn_compare(&result, &one) != 0) return false;
    }

    return true;
}

/**
 * Gauss Euler primality test. Link: https://arxiv.org/pdf/2311.07048
 *
 * @param potential_prime prime to check
 * @param config config
 * @return true if probably prime
 */
bool gauss_euler(BigNum* potential_prime, Config* config) {
    BigNum one, two;
    bn_init(&one); one.words[0] = 1;
    bn_init(&two); two.words[0] = 2;

    if (bn_compare(potential_prime, &two) == 0) return true;
    if (bn_is_even(potential_prime) || bn_compare(potential_prime, &two) < 0) return false;

    // Determine multiplication method
    int method = config->mul_method;
    if (method == MUL_AUTO) {
        method = bn_auto_select_mul_method(potential_prime, potential_prime);
    }

    BigNum n_minus_1, exp, t;
    bn_sub(&n_minus_1, potential_prime, &one);
    bn_copy(&exp, &n_minus_1);
    bn_shift_right(&exp, 1);  // (n-1)/2

    // Euler criterion for 2
    bn_mod_pow(&t, &two, &exp, potential_prime, method);

    // Check n % 8
    uint32_t n_mod_8 = potential_prime->words[0] & 7;
    if ((n_mod_8 == 1 || n_mod_8 == 7) && bn_compare(&t, &one) != 0) {
        return false;
    }
    if ((n_mod_8 == 3 || n_mod_8 == 5) && bn_compare(&t, &n_minus_1) != 0) {
        return false;
    }

    // For large BigNums, skip the expensive sqrt-based checks
    // This is a simplified version

    // Find first prime p1 ≡ 5 (mod 8) where n is not a quadratic residue
    BigNum p1, five, eight;
    bn_init(&five); five.words[0] = 5;
    bn_init(&eight); eight.words[0] = 8;
    bn_copy(&p1, &five);

    // Simplified: just test a few small primes
    BigNum test_primes[3];
    bn_init(&test_primes[0]); test_primes[0].words[0] = 5;
    bn_init(&test_primes[1]); test_primes[1].words[0] = 13;
    bn_init(&test_primes[2]); test_primes[2].words[0] = 29;

    for (int i = 0; i < 3; i++) {
        BigNum result;
        bn_mod_pow(&result, &test_primes[i], &exp, potential_prime, method);
        if (bn_compare(&result, &n_minus_1) != 0) {
            return false;
        }
    }

    return true;
}

/**
 * Miller-Rabin and Gauss-Euler primality test.
 * Hybrid approach combining both methods for enhanced primality testing.
 *
 * Link: https://arxiv.org/pdf/2311.07048
 *
 * @param potential_prime the number to check primality of.
 * @param config config.
 * @return true or false if the number is prime.
 */
bool mr_ge(BigNum* potential_prime, Config* config) {
    BigNum two, three, five, seven;
    bn_init(&two); two.words[0] = 2;
    bn_init(&three); three.words[0] = 3;
    bn_init(&five); five.words[0] = 5;
    bn_init(&seven); seven.words[0] = 7;

    if (bn_compare(potential_prime, &two) == 0 ||
        bn_compare(potential_prime, &three) == 0 ||
        bn_compare(potential_prime, &five) == 0 ||
        bn_compare(potential_prime, &seven) == 0) {
        return true;
    }
    if (bn_compare(potential_prime, &two) < 0 || bn_is_even(potential_prime)) {
        return false;
    }

    // Determine multiplication method
    int method = config->mul_method;
    if (method == MUL_AUTO) {
        method = bn_auto_select_mul_method(potential_prime, potential_prime);
    }

    BigNum one, n_minus_1;
    bn_init(&one); one.words[0] = 1;
    bn_sub(&n_minus_1, potential_prime, &one);

    // Factor out powers of 2: n - 1 = 2^s * t
    BigNum t;
    bn_copy(&t, &n_minus_1);
    int s = 0;
    while (bn_is_even(&t)) {
        s++;
        bn_shift_right(&t, 1);
    }

    // Test with base 2 (simplified version - full version would compute sqrt(n))
    BigNum prime_bases[1];
    bn_init(&prime_bases[0]); prime_bases[0].words[0] = 2;

    for (size_t x = 0; x < 1; x++) {
        BigNum b;
        bn_mod_pow(&b, &prime_bases[x], &t, potential_prime, method);

        for (int y = 1; y <= s; y++) {
            BigNum k;
            bn_mod_mul(&k, &b, &b, potential_prime, method);

            if (bn_compare(&k, &one) == 0 &&
                bn_compare(&b, &one) != 0 &&
                bn_compare(&b, &n_minus_1) != 0) {
                return false;
            }
            bn_copy(&b, &k);
        }

        if (bn_compare(&b, &one) != 0) {
            return false;
        }
    }

    // Simplified quadratic residue test with small primes
    BigNum test_primes[2];
    bn_init(&test_primes[0]); test_primes[0].words[0] = 3;
    bn_init(&test_primes[1]); test_primes[1].words[0] = 7;

    BigNum exp;
    bn_copy(&exp, &n_minus_1);
    bn_shift_right(&exp, 1);  // (n-1)/2

    for (int i = 0; i < 2; i++) {
        BigNum d;
        bn_mod_pow(&d, &test_primes[i], &exp, potential_prime, method);

        uint32_t n_mod_4 = potential_prime->words[0] & 3;
        if ((n_mod_4 == 1 && bn_compare(&d, &n_minus_1) != 0) ||
            (n_mod_4 == 3 && bn_compare(&d, &one) != 0)) {
            return false;
        }
    }

    return true;
}