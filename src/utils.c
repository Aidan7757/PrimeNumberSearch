#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

#include "bn.h"

// Internal: Convert multiplication method enum to BigNum method
static int config_to_bn_method(MultiplicationMethod method) {
    switch (method) {
        case MUL_BIGNUM_STANDARD:    return MUL_METHOD_STANDARD;
        case MUL_BIGNUM_KARATSUBA:   return MUL_METHOD_KARATSUBA;
        case MUL_BIGNUM_SS:          return MUL_METHOD_SCHONHAGE_STRASSEN;
        case MUL_AUTO:               return MUL_METHOD_KARATSUBA; // Safe default
        default:                     return MUL_METHOD_STANDARD;
    }
}

// Wrapper: mod_mul with config-based method selection
__int128_t mod_mul(__int128_t a, __int128_t b, __int128_t mod, Config* config) {
    // Fast path: native __int128_t multiplication
    if (config->mul_method == MUL_NATIVE_INT128 || !config->use_bignum) {
        return (a % mod) * (b % mod) % mod;
    }

    // BigNum path
    BigNum bn_a, bn_b, bn_mod, bn_result;
    bn_from_int128(&bn_a, a);
    bn_from_int128(&bn_b, b);
    bn_from_int128(&bn_mod, mod);

    int bn_method = config_to_bn_method(config->mul_method);
    bn_mod_mul(&bn_result, &bn_a, &bn_b, &bn_mod, bn_method);

    __int128_t result = bn_to_int128(&bn_result);

    // Optional verification
    if (config->verify_results) {
        __int128_t native_result = (a % mod) * (b % mod) % mod;
        if (result != native_result) {
            fprintf(stderr, "WARNING: BigNum result mismatch! Native: %lld, BigNum: %lld\n",
                    (long long)native_result, (long long)result);
        }
    }

    return result;
}

// Wrapper: mod_pow with config-based method selection
__int128_t mod_pow(__int128_t base, __int128_t exp, __int128_t mod,  Config* config) {
    // Fast path: native __int128_t
    if (config->mul_method == MUL_NATIVE_INT128 || !config->use_bignum) {
        __int128_t result = 1;
        base %= mod;

        while (exp > 0) {
            if (exp & 1) {
                result = mod_mul(result, base, mod, config);
            }
            base = mod_mul(base, base, mod, config);
            exp >>= 1;
        }

        return result;
    }

    // BigNum path
    BigNum bn_base, bn_exp, bn_mod, bn_result;
    bn_from_int128(&bn_base, base);
    bn_from_int128(&bn_exp, exp);
    bn_from_int128(&bn_mod, mod);

    int bn_method = config_to_bn_method(config->mul_method);
    bn_mod_pow(&bn_result, &bn_base, &bn_exp, &bn_mod, bn_method);

    return bn_to_int128(&bn_result);
}

// Factor out powers of 2: n - 1 = 2^s * d
long factor_out_twos(__int128_t n, __int128_t* d) {
    *d = n - 1;
    long s = 0;

    while ((*d & 1) == 0) {
        *d >>= 1;
        s++;
    }

    return s;
}