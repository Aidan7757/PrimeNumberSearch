#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "models.h"  // This brings in the Config typedef

// Use Config, NOT struct Config
__int128_t mod_mul(__int128_t a, __int128_t b, __int128_t mod, Config* config);
__int128_t mod_pow(__int128_t base, __int128_t exp, __int128_t mod, Config* config);
long factor_out_twos(__int128_t n, __int128_t* d);

#endif // UTILS_H