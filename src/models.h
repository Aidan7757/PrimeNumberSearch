#ifndef MODELS_H
#define MODELS_H

#include <stdbool.h>
#include <stdint.h>

// Define the enum
typedef enum {
    MUL_NATIVE_INT128 = 0,
    MUL_BIGNUM_STANDARD = 1,
    MUL_BIGNUM_KARATSUBA = 2,
    MUL_BIGNUM_SS = 3,
    MUL_AUTO = 4
} MultiplicationMethod;

// Define Config as a typedef (NOT as 'struct Config')
typedef struct {
    int num_threads;
    __int128_t lower_range;
    __int128_t max_range;
    int num_rounds;
    MultiplicationMethod mul_method;
    bool enable_fft_ss;
    bool use_bignum;
    int bignum_bit_size;
    bool verbose;
    bool verify_results;
} Config;

void config_init(Config* config);
void config_print(const Config* config);
const char* config_get_mul_method_name(MultiplicationMethod method);

#endif // MODELS_H