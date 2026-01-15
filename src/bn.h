#ifndef BIGNUM_H
#define BIGNUM_H

#include <stdint.h>
#include <stdbool.h>

// BigNum structure for arbitrary precision arithmetic
#define MAX_BIGNUM_WORDS 512  // Support up to 16,384 bits (512 * 32)
#define WORD_BITS 32
#define WORD_MASK 0xFFFFFFFF

typedef struct {
    uint32_t words[MAX_BIGNUM_WORDS];
    int num_words;  // Number of significant words
    bool negative;
} BigNum;

// ==================== INITIALIZATION & CONVERSION ====================

/**
 * Initialize a BigNum to zero
 * @param bn BigNum to initialize
 */
void bn_init(BigNum* bn);

/**
 * Create BigNum from __int128_t value
 * @param bn Destination BigNum
 * @param value Source __int128_t value
 */
void bn_from_int128(BigNum* bn, __int128_t value);

/**
 * Create BigNum from hexadecimal string
 * @param bn Destination BigNum
 * @param hex_string Hex string (with or without "0x" prefix)
 */
void bn_from_string(BigNum* bn, const char* hex_string);

/**
 * Copy one BigNum to another
 * @param dest Destination BigNum
 * @param src Source BigNum
 */
void bn_copy(BigNum* dest, const BigNum* src);

/**
 * Convert BigNum to __int128_t (if it fits)
 * @param bn Source BigNum
 * @return __int128_t value, or 0 if too large
 */
__int128_t bn_to_int128(const BigNum* bn);

// ==================== COMPARISON & DISPLAY ====================

/**
 * Compare two BigNums
 * @param a First BigNum
 * @param b Second BigNum
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int bn_compare(const BigNum* a, const BigNum* b);

/**
 * Print BigNum in hexadecimal format with label
 * @param bn BigNum to print
 * @param label Label string to display
 */
void bn_print(const BigNum* bn, const char* label);

/**
 * Check if BigNum is zero
 * @param bn BigNum to check
 * @return true if zero, false otherwise
 */
bool bn_is_zero(const BigNum* bn);

/**
 * Check if BigNum is one
 * @param bn BigNum to check
 * @return true if one, false otherwise
 */
bool bn_is_one(const BigNum* bn);

/**
 * Check if BigNum is even
 * @param bn BigNum to check
 * @return true if even, false otherwise
 */
bool bn_is_even(const BigNum* bn);

// ==================== BASIC ARITHMETIC ====================

/**
 * Add two BigNums: result = a + b
 * @param result Destination for sum
 * @param a First operand
 * @param b Second operand
 */
void bn_add(BigNum* result, const BigNum* a, const BigNum* b);

/**
 * Subtract two BigNums: result = a - b
 * @param result Destination for difference
 * @param a First operand (minuend)
 * @param b Second operand (subtrahend)
 */
void bn_sub(BigNum* result, const BigNum* a, const BigNum* b);

/**
 * Multiply two BigNums using standard O(n²) algorithm
 * Best for small numbers (< 1000 bits)
 * @param result Destination for product
 * @param a First operand
 * @param b Second operand
 */
void bn_mul_standard(BigNum* result, const BigNum* a, const BigNum* b);

/**
 * Multiply two BigNums using Karatsuba algorithm O(n^1.585)
 * Best for medium numbers (1000-8000 bits)
 * @param result Destination for product
 * @param a First operand
 * @param b Second operand
 */
void bn_mul_karatsuba(BigNum* result, const BigNum* a, const BigNum* b);

/**
 * Multiply two BigNums using Schönhage-Strassen algorithm O(n log n log log n)
 * Best for large numbers (> 8000 bits)
 * @param result Destination for product
 * @param a First operand
 * @param b Second operand
 */
void bn_mul_ss(BigNum* result, const BigNum* a, const BigNum* b);

/**
 * Divide two BigNums: a = quotient * b + remainder
 * @param quotient Destination for quotient (can be NULL)
 * @param remainder Destination for remainder (can be NULL)
 * @param a Dividend
 * @param b Divisor
 */
void bn_div(BigNum* quotient, BigNum* remainder, const BigNum* a, const BigNum* b);

/**
 * Modulo operation: result = a % mod
 * @param result Destination for result
 * @param a Operand
 * @param mod Modulus
 */
void bn_mod(BigNum* result, const BigNum* a, const BigNum* mod);

// ==================== MODULAR ARITHMETIC ====================

/**
 * Modular multiplication: result = (a * b) % mod
 * @param result Destination for result
 * @param a First operand
 * @param b Second operand
 * @param mod Modulus
 * @param method Multiplication method (MUL_METHOD_STANDARD, etc.)
 */
void bn_mod_mul(BigNum* result, const BigNum* a, const BigNum* b,
                const BigNum* mod, int method);

/**
 * Modular exponentiation: result = (base^exp) % mod
 * Uses square-and-multiply algorithm
 * @param result Destination for result
 * @param base Base
 * @param exp Exponent
 * @param mod Modulus
 * @param method Multiplication method for intermediate operations
 */
void bn_mod_pow(BigNum* result, const BigNum* base, const BigNum* exp,
                const BigNum* mod, int method);

/**
 * Modular addition: result = (a + b) % mod
 * @param result Destination for result
 * @param a First operand
 * @param b Second operand
 * @param mod Modulus
 */
void bn_mod_add(BigNum* result, const BigNum* a, const BigNum* b, const BigNum* mod);

/**
 * Modular subtraction: result = (a - b) % mod
 * @param result Destination for result
 * @param a First operand
 * @param b Second operand
 * @param mod Modulus
 */
void bn_mod_sub(BigNum* result, const BigNum* a, const BigNum* b, const BigNum* mod);

// ==================== BIT OPERATIONS ====================

/**
 * Shift BigNum left by specified number of bits
 * @param bn BigNum to shift (modified in place)
 * @param bits Number of bits to shift
 */
void bn_shift_left(BigNum* bn, int bits);

/**
 * Shift BigNum right by specified number of bits
 * @param bn BigNum to shift (modified in place)
 * @param bits Number of bits to shift
 */
void bn_shift_right(BigNum* bn, int bits);

/**
 * Get the bit at specified position
 * @param bn BigNum to query
 * @param bit_position Bit position (0 = LSB)
 * @return 1 if bit is set, 0 otherwise
 */
int bn_get_bit(const BigNum* bn, int bit_position);

/**
 * Set the bit at specified position
 * @param bn BigNum to modify
 * @param bit_position Bit position (0 = LSB)
 * @param value Value to set (0 or 1)
 */
void bn_set_bit(BigNum* bn, int bit_position, int value);

/**
 * Count the number of bits in the BigNum
 * @param bn BigNum to measure
 * @return Number of bits
 */
int bn_bit_length(const BigNum* bn);

// ==================== UTILITY FUNCTIONS ====================

/**
 * Generate random BigNum with specified bit length
 * @param bn Destination BigNum
 * @param bits Number of bits (must be > 0)
 */
void bn_random(BigNum* bn, int bits);

/**
 * Generate random BigNum in range [min, max)
 * @param bn Destination BigNum
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 */
void bn_random_range(BigNum* bn, const BigNum* min, const BigNum* max);

/**
 * Compute GCD of two BigNums using Euclidean algorithm
 * @param result Destination for GCD
 * @param a First operand
 * @param b Second operand
 */
void bn_gcd(BigNum* result, const BigNum* a, const BigNum* b);

/**
 * Check if BigNum is probably prime using Miller-Rabin
 * @param n Number to test
 * @param rounds Number of test rounds (more = higher confidence)
 * @param mul_method Multiplication method to use
 * @return true if probably prime, false if composite
 */
bool bn_is_prime_mr(const BigNum* n, int rounds, int mul_method);

// ==================== MULTIPLICATION METHOD CONSTANTS ====================

#define MUL_METHOD_STANDARD 0           // O(n²) - Best for < 1000 bits
#define MUL_METHOD_KARATSUBA 1          // O(n^1.585) - Best for 1000-8000 bits
#define MUL_METHOD_SCHONHAGE_STRASSEN 2 // O(n log n log log n) - Best for > 8000 bits

/**
 * Automatically select best multiplication method based on size
 * @param a First operand
 * @param b Second operand
 * @return Best multiplication method constant
 */
int bn_auto_select_mul_method(const BigNum* a, const BigNum* b);

// ==================== ERROR CODES ====================

#define BN_SUCCESS 0
#define BN_ERROR_OVERFLOW -1
#define BN_ERROR_DIVISION_BY_ZERO -2
#define BN_ERROR_INVALID_INPUT -3

/**
 * Get error message for error code
 * @param error_code Error code
 * @return Human-readable error message
 */
const char* bn_get_error_message(int error_code);

#endif // BIGNUM_H