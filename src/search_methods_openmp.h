#pragma once
#include <stdbool.h>
#include "models.h"

/**
 * Naive check for if a number is prime. Non probablistic.
 * @param potential_prime long: potential prime number to be checked.
 * @param config config
 * @return true or false on if the number is detected to be prime or not.
 */
bool naive_check(long long potential_prime, struct Config* config);

/**
 * Miller-Rabin primality test, probabilistic primality test.
 * Link: https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test
 *
 * @param potential_prime prime to check.
 * @param config settings.
 * @return true or false on if likely to be prime or not.
 */
bool miller_rabin(long long potential_prime, struct Config* config);

/**
 * Fermat primality test, probabilistic primality test.
 * Link: https://en.wikipedia.org/wiki/Fermat_primality_test
 *
 * @param potential_prime the number to check primality of.
 * @param config struct config.
 * @return true or false if the number is prime.
 */
bool fermat(long long potential_prime, struct Config* config);

/**
 * Gauss Euler primality test. Link: https://arxiv.org/pdf/2311.07048
 *
 * @param potential_prime prime to check
 * @param config

 * @return
 */
bool gauss_euler(long long potential_prime, struct Config* config);


/**
 * Miller-Rabin and Gauss-Euler primality test.
 * Hybrid approach combining both methods for enhanced primality testing.
 *
 * Link: https://arxiv.org/pdf/2311.07048
 *
 * @param potential_prime the number to check primality of.
 * @param config struct config.
 * @return true or false if the number is prime.
 */
bool mr_ge(long long potential_prime, struct Config* config);