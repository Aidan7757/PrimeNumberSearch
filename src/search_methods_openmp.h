#ifndef SEARCH_METHODS_OPENMP_H
#define SEARCH_METHODS_OPENMP_H

#include <stdbool.h>
#include <stdint.h>
#include "models.h"  // This brings in Config typedef
#include "bn.h"      // This brings in BigNum typedef

// Use BigNum* instead of __int128_t
typedef bool (*PrimeTestFunc)(BigNum*, Config*);

bool naive_check(BigNum* potential_prime, Config* config);
bool miller_rabin(BigNum* potential_prime, Config* config);
bool fermat(BigNum* potential_prime, Config* config);
bool gauss_euler(BigNum* potential_prime, Config* config);
bool mr_ge(BigNum* potential_prime, Config* config);

#endif // SEARCH_METHODS_OPENMP_H