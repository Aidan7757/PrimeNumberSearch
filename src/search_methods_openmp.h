#ifndef SEARCH_METHODS_OPENMP_H
#define SEARCH_METHODS_OPENMP_H

#include <stdbool.h>
#include <stdint.h>
#include "models.h"  // This brings in Config typedef

// Use Config, NOT struct Config
typedef bool (*PrimeTestFunc)(__int128_t, Config*);

bool naive_check(__int128_t potential_prime, Config* config);
bool miller_rabin(__int128_t potential_prime, Config* config);
bool fermat(__int128_t potential_prime, Config* config);
bool gauss_euler(__int128_t potential_prime, Config* config);
bool mr_ge(__int128_t potential_prime, Config* config);

#endif // SEARCH_METHODS_OPENMP_H