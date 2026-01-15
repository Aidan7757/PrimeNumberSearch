#include "models.h"
#include <stdio.h>

void config_init(Config* config) {
    config->num_threads = 8;

    // 2048-bit numbers (good for seeing Karatsuba advantage)
    bn_from_string(&config->lower_range, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");

    bn_from_string(&config->max_range, "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000");

    // Or simpler hex notation:
    // bn_from_string(&config->lower_range, "0x1" followed by 512 zeros);

    config->num_rounds = 6;
    config->mul_method = MUL_AUTO;
    config->use_bignum = true;
    config->bignum_bit_size = 2048;  // For Karatsuba advantage
    config->verbose = false;
    config->verify_results = false;
}

void config_print(const Config* config) {
    // printf("Configuration:\n");
    // printf("  Threads:          %d\n", config->num_threads);
    // printf("  Range:            %lld to %lld\n",
    //        (long long)config->lower_range,
    //        (long long)config->max_range);
    // printf("  Rounds:           %d\n", config->num_rounds);
    // printf("  Mul Method:       %s\n", config_get_mul_method_name(config->mul_method));
    // printf("  Use BigNum:       %s\n", config->use_bignum ? "Yes" : "No");
    // if (config->use_bignum) {
    //     printf("  BigNum Bit Size:  %d %s\n",
    //            config->bignum_bit_size,
    //            config->bignum_bit_size == 0 ? "(auto)" : "");
    // }
    // printf("  Verbose:          %s\n", config->verbose ? "Yes" : "No");
    // printf("  Verify Results:   %s\n", config->verify_results ? "Yes" : "No");
}

const char* config_get_mul_method_name(MultiplicationMethod method) {
    switch (method) {
        case MUL_NATIVE_INT128:      return "Native __int128_t";
        case MUL_BIGNUM_STANDARD:    return "BigNum Standard O(n²)";
        case MUL_BIGNUM_KARATSUBA:   return "BigNum Karatsuba O(n^1.585)";
        case MUL_BIGNUM_SS:          return "BigNum Schönhage-Strassen";
        case MUL_AUTO:               return "Auto-select";
        default:                     return "Unknown";
    }
}