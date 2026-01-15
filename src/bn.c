#include "bn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================== INITIALIZATION & CONVERSION ====================

void bn_init(BigNum* bn) {
    memset(bn->words, 0, sizeof(bn->words));
    bn->num_words = 1;
    bn->negative = false;
}

void bn_from_int128(BigNum* bn, __int128_t value) {
    bn_init(bn);

    if (value < 0) {
        bn->negative = true;
        value = -value;
    }

    int word_idx = 0;
    while (value > 0 && word_idx < MAX_BIGNUM_WORDS) {
        bn->words[word_idx++] = (uint32_t)(value & WORD_MASK);
        value >>= WORD_BITS;
    }
    bn->num_words = (word_idx == 0) ? 1 : word_idx;
}

void bn_from_string(BigNum* bn, const char* hex_string) {
    bn_init(bn);

    // Skip "0x" prefix if present
    if (hex_string[0] == '0' && (hex_string[1] == 'x' || hex_string[1] == 'X')) {
        hex_string += 2;
    }

    int len = strlen(hex_string);
    int word_idx = 0;

    for (int i = len - 1; i >= 0 && word_idx < MAX_BIGNUM_WORDS; i -= 8) {
        uint32_t word = 0;
        int start = (i - 7 < 0) ? 0 : i - 7;
        for (int j = start; j <= i; j++) {
            char c = hex_string[j];
            int digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else continue;
            word = (word << 4) | digit;
        }
        bn->words[word_idx++] = word;
    }
    bn->num_words = (word_idx == 0) ? 1 : word_idx;
}

void bn_copy(BigNum* dest, const BigNum* src) {
    memcpy(dest->words, src->words, sizeof(src->words));
    dest->num_words = src->num_words;
    dest->negative = src->negative;
}

__int128_t bn_to_int128(const BigNum* bn) {
    if (bn->num_words > 4) return 0; // Too large

    __int128_t result = 0;
    for (int i = bn->num_words - 1; i >= 0; i--) {
        result = (result << WORD_BITS) | bn->words[i];
    }

    return bn->negative ? -result : result;
}

// ==================== COMPARISON & DISPLAY ====================

int bn_compare(const BigNum* a, const BigNum* b) {
    if (a->negative != b->negative) {
        return a->negative ? -1 : 1;
    }

    if (a->num_words != b->num_words) {
        int cmp = (a->num_words > b->num_words) ? 1 : -1;
        return a->negative ? -cmp : cmp;
    }

    for (int i = a->num_words - 1; i >= 0; i--) {
        if (a->words[i] != b->words[i]) {
            int cmp = (a->words[i] > b->words[i]) ? 1 : -1;
            return a->negative ? -cmp : cmp;
        }
    }
    return 0;
}

void bn_print(const BigNum* bn, const char* label) {
    printf("%s: ", label);
    if (bn->negative) printf("-");
    printf("0x");
    bool leading = true;
    for (int i = bn->num_words - 1; i >= 0; i--) {
        if (leading && bn->words[i] == 0 && i > 0) continue;
        leading = false;
        printf("%08X", bn->words[i]);
    }
    printf(" (%d words, %d bits)\n", bn->num_words, bn_bit_length(bn));
}

bool bn_is_zero(const BigNum* bn) {
    for (int i = 0; i < bn->num_words; i++) {
        if (bn->words[i] != 0) return false;
    }
    return true;
}

bool bn_is_one(const BigNum* bn) {
    if (bn->negative || bn->words[0] != 1) return false;
    for (int i = 1; i < bn->num_words; i++) {
        if (bn->words[i] != 0) return false;
    }
    return true;
}

bool bn_is_even(const BigNum* bn) {
    return (bn->words[0] & 1) == 0;
}

// ==================== BASIC ARITHMETIC ====================

void bn_add(BigNum* result, const BigNum* a, const BigNum* b) {
    if (a->negative != b->negative) {
        // Handle subtraction case
        BigNum temp;
        bn_copy(&temp, b);
        temp.negative = !temp.negative;
        bn_sub(result, a, &temp);
        return;
    }

    uint64_t carry = 0;
    int max_words = (a->num_words > b->num_words) ? a->num_words : b->num_words;

    for (int i = 0; i < max_words || carry; i++) {
        if (i >= MAX_BIGNUM_WORDS) break;

        uint64_t sum = carry;
        if (i < a->num_words) sum += a->words[i];
        if (i < b->num_words) sum += b->words[i];

        result->words[i] = (uint32_t)(sum & WORD_MASK);
        carry = sum >> WORD_BITS;
    }

    result->num_words = max_words;
    if (carry && max_words < MAX_BIGNUM_WORDS) {
        result->words[max_words++] = (uint32_t)carry;
    }
    result->num_words = max_words;
    result->negative = a->negative;
}

void bn_sub(BigNum* result, const BigNum* a, const BigNum* b) {
    if (a->negative != b->negative) {
        // Handle addition case
        BigNum temp;
        bn_copy(&temp, b);
        temp.negative = !temp.negative;
        bn_add(result, a, &temp);
        return;
    }

    // Ensure a >= b for positive subtraction
    if (bn_compare(a, b) < 0) {
        bn_sub(result, b, a);
        result->negative = !result->negative;
        return;
    }

    int64_t borrow = 0;
    for (int i = 0; i < a->num_words; i++) {
        int64_t diff = (int64_t)a->words[i] - borrow;
        if (i < b->num_words) diff -= b->words[i];

        if (diff < 0) {
            diff += ((int64_t)1 << WORD_BITS);
            borrow = 1;
        } else {
            borrow = 0;
        }
        result->words[i] = (uint32_t)diff;
    }

    // Find actual number of words
    int num_words = a->num_words;
    while (num_words > 1 && result->words[num_words - 1] == 0) {
        num_words--;
    }
    result->num_words = num_words;
    result->negative = a->negative;
}

void bn_mul_standard(BigNum* result, const BigNum* a, const BigNum* b) {
    bn_init(result);

    for (int i = 0; i < a->num_words; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < b->num_words; j++) {
            if (i + j >= MAX_BIGNUM_WORDS) break;

            uint64_t prod = (uint64_t)a->words[i] * b->words[j];
            uint64_t sum = result->words[i + j] + prod + carry;

            result->words[i + j] = (uint32_t)(sum & WORD_MASK);
            carry = sum >> WORD_BITS;
        }

        if (i + b->num_words < MAX_BIGNUM_WORDS && carry) {
            result->words[i + b->num_words] += (uint32_t)carry;
        }
    }

    result->num_words = a->num_words + b->num_words;
    while (result->num_words > 1 && result->words[result->num_words - 1] == 0) {
        result->num_words--;
    }
    result->negative = (a->negative != b->negative);
}

void bn_mul_karatsuba(BigNum* result, const BigNum* a, const BigNum* b) {
    // Base case: use standard multiplication for small numbers
    if (a->num_words <= 8 || b->num_words <= 8) {
        bn_mul_standard(result, a, b);
        return;
    }

    int m = (a->num_words > b->num_words ? a->num_words : b->num_words) / 2;

    BigNum low1, high1, low2, high2;
    bn_init(&low1);
    bn_init(&high1);
    bn_init(&low2);
    bn_init(&high2);

    // Split a = high1 * B^m + low1
    memcpy(low1.words, a->words, m * sizeof(uint32_t));
    low1.num_words = m;
    while (low1.num_words > 1 && low1.words[low1.num_words - 1] == 0) {
        low1.num_words--;
    }

    if (a->num_words > m) {
        memcpy(high1.words, a->words + m, (a->num_words - m) * sizeof(uint32_t));
        high1.num_words = a->num_words - m;
    }

    // Split b = high2 * B^m + low2
    memcpy(low2.words, b->words, m * sizeof(uint32_t));
    low2.num_words = m;
    while (low2.num_words > 1 && low2.words[low2.num_words - 1] == 0) {
        low2.num_words--;
    }

    if (b->num_words > m) {
        memcpy(high2.words, b->words + m, (b->num_words - m) * sizeof(uint32_t));
        high2.num_words = b->num_words - m;
    }

    // z0 = low1 * low2
    BigNum z0;
    bn_mul_karatsuba(&z0, &low1, &low2);

    // z2 = high1 * high2
    BigNum z2;
    bn_mul_karatsuba(&z2, &high1, &high2);

    // z1 = (low1 + high1) * (low2 + high2) - z0 - z2
    BigNum sum1, sum2, z1;
    bn_add(&sum1, &low1, &high1);
    bn_add(&sum2, &low2, &high2);
    bn_mul_karatsuba(&z1, &sum1, &sum2);
    bn_sub(&z1, &z1, &z0);
    bn_sub(&z1, &z1, &z2);

    // result = z2 * B^(2m) + z1 * B^m + z0
    bn_copy(result, &z0);
    bn_shift_left(&z1, m * WORD_BITS);
    bn_add(result, result, &z1);
    bn_shift_left(&z2, 2 * m * WORD_BITS);
    bn_add(result, result, &z2);

    result->negative = (a->negative != b->negative);
}

void bn_mul_ss(BigNum* result, const BigNum* a, const BigNum* b) {
    // For now, use Karatsuba as SS requires proper FFT implementation
    // SS only becomes better for numbers > 10,000 bits typically
    if (a->num_words < 256 || b->num_words < 256) {
        bn_mul_karatsuba(result, a, b);
        return;
    }

    // TODO: Implement proper FFT-based Schönhage-Strassen
    // For now, fall back to Karatsuba
    bn_mul_karatsuba(result, a, b);
}

void bn_div(BigNum* quotient, BigNum* remainder, const BigNum* a, const BigNum* b) {
    if (bn_is_zero(b)) {
        fprintf(stderr, "Error: Division by zero\n");
        if (quotient) bn_init(quotient);
        if (remainder) bn_init(remainder);
        return;
    }

    if (quotient) bn_init(quotient);
    BigNum rem;
    bn_copy(&rem, a);

    if (bn_compare(a, b) < 0) {
        if (remainder) bn_copy(remainder, a);
        return;
    }

    // Long division algorithm
    BigNum current, temp;
    bn_init(&current);

    int bits = bn_bit_length(a);
    for (int i = bits - 1; i >= 0; i--) {
        bn_shift_left(&current, 1);
        if (bn_get_bit(a, i)) {
            current.words[0] |= 1;
        }

        if (bn_compare(&current, b) >= 0) {
            bn_sub(&current, &current, b);
            if (quotient) {
                bn_set_bit(quotient, i, 1);
            }
        }
    }

    if (remainder) bn_copy(remainder, &current);

    if (quotient) {
        quotient->num_words = (bits + WORD_BITS - 1) / WORD_BITS;
        while (quotient->num_words > 1 && quotient->words[quotient->num_words - 1] == 0) {
            quotient->num_words--;
        }
    }
}

void bn_mod(BigNum* result, const BigNum* a, const BigNum* mod) {
    bn_div(NULL, result, a, mod);
}

// ==================== MODULAR ARITHMETIC ====================

void bn_mod_mul(BigNum* result, const BigNum* a, const BigNum* b,
                const BigNum* mod, int method) {
    BigNum temp;

    switch(method) {
        case MUL_METHOD_KARATSUBA:
            bn_mul_karatsuba(&temp, a, b);
            break;
        case MUL_METHOD_SCHONHAGE_STRASSEN:
            bn_mul_ss(&temp, a, b);
            break;
        default:
            bn_mul_standard(&temp, a, b);
            break;
    }

    bn_mod(result, &temp, mod);
}

void bn_mod_pow(BigNum* result, const BigNum* base, const BigNum* exp,
                const BigNum* mod, int method) {
    BigNum res, b, e;
    bn_init(&res);
    res.words[0] = 1;

    bn_mod(&b, base, mod);
    bn_copy(&e, exp);

    while (!bn_is_zero(&e)) {
        if (e.words[0] & 1) {
            bn_mod_mul(&res, &res, &b, mod, method);
        }
        bn_mod_mul(&b, &b, &b, mod, method);
        bn_shift_right(&e, 1);
    }

    bn_copy(result, &res);
}

void bn_mod_add(BigNum* result, const BigNum* a, const BigNum* b, const BigNum* mod) {
    bn_add(result, a, b);
    if (bn_compare(result, mod) >= 0) {
        bn_sub(result, result, mod);
    }
}

void bn_mod_sub(BigNum* result, const BigNum* a, const BigNum* b, const BigNum* mod) {
    if (bn_compare(a, b) >= 0) {
        bn_sub(result, a, b);
    } else {
        BigNum temp;
        bn_sub(&temp, mod, b);
        bn_add(result, a, &temp);
        bn_mod(result, result, mod);
    }
}

// ==================== BIT OPERATIONS ====================

void bn_shift_left(BigNum* bn, int bits) {
    if (bits == 0) return;

    int word_shift = bits / WORD_BITS;
    int bit_shift = bits % WORD_BITS;

    if (word_shift > 0) {
        for (int i = bn->num_words - 1; i >= 0; i--) {
            if (i + word_shift < MAX_BIGNUM_WORDS) {
                bn->words[i + word_shift] = bn->words[i];
            }
        }
        for (int i = 0; i < word_shift && i < MAX_BIGNUM_WORDS; i++) {
            bn->words[i] = 0;
        }
        bn->num_words += word_shift;
        if (bn->num_words > MAX_BIGNUM_WORDS) {
            bn->num_words = MAX_BIGNUM_WORDS;
        }
    }

    if (bit_shift > 0) {
        uint32_t carry = 0;
        for (int i = word_shift; i < bn->num_words; i++) {
            uint32_t new_carry = bn->words[i] >> (WORD_BITS - bit_shift);
            bn->words[i] = (bn->words[i] << bit_shift) | carry;
            carry = new_carry;
        }
        if (carry && bn->num_words < MAX_BIGNUM_WORDS) {
            bn->words[bn->num_words++] = carry;
        }
    }
}

void bn_shift_right(BigNum* bn, int bits) {
    if (bits == 0) return;

    int word_shift = bits / WORD_BITS;
    int bit_shift = bits % WORD_BITS;

    if (word_shift >= bn->num_words) {
        bn_init(bn);
        return;
    }

    if (word_shift > 0) {
        for (int i = 0; i < bn->num_words - word_shift; i++) {
            bn->words[i] = bn->words[i + word_shift];
        }
        bn->num_words -= word_shift;
    }

    if (bit_shift > 0) {
        for (int i = 0; i < bn->num_words - 1; i++) {
            bn->words[i] = (bn->words[i] >> bit_shift) |
                          (bn->words[i + 1] << (WORD_BITS - bit_shift));
        }
        bn->words[bn->num_words - 1] >>= bit_shift;
    }

    while (bn->num_words > 1 && bn->words[bn->num_words - 1] == 0) {
        bn->num_words--;
    }
}

int bn_get_bit(const BigNum* bn, int bit_position) {
    int word_idx = bit_position / WORD_BITS;
    int bit_idx = bit_position % WORD_BITS;

    if (word_idx >= bn->num_words) return 0;

    return (bn->words[word_idx] >> bit_idx) & 1;
}

void bn_set_bit(BigNum* bn, int bit_position, int value) {
    int word_idx = bit_position / WORD_BITS;
    int bit_idx = bit_position % WORD_BITS;

    if (word_idx >= MAX_BIGNUM_WORDS) return;

    if (word_idx >= bn->num_words) {
        bn->num_words = word_idx + 1;
    }

    if (value) {
        bn->words[word_idx] |= (1U << bit_idx);
    } else {
        bn->words[word_idx] &= ~(1U << bit_idx);
    }
}

int bn_bit_length(const BigNum* bn) {
    if (bn->num_words == 0 || bn_is_zero(bn)) return 0;

    int top_word = bn->words[bn->num_words - 1];
    int bits = (bn->num_words - 1) * WORD_BITS;

    while (top_word > 0) {
        bits++;
        top_word >>= 1;
    }

    return bits;
}

// ==================== UTILITY FUNCTIONS ====================

void bn_random(BigNum* bn, int bits) {
    bn_init(bn);
    int words = (bits + WORD_BITS - 1) / WORD_BITS;

    static bool seeded = false;
    if (!seeded) {
        srand(time(NULL));
        seeded = true;
    }

    for (int i = 0; i < words && i < MAX_BIGNUM_WORDS; i++) {
        bn->words[i] = (uint32_t)rand() ^ ((uint32_t)rand() << 15);
    }
    bn->num_words = words;

    // Mask off excess bits in top word
    int excess_bits = (words * WORD_BITS) - bits;
    if (excess_bits > 0 && words > 0) {
        bn->words[words - 1] &= (1U << (WORD_BITS - excess_bits)) - 1;
    }

    // Ensure top bit is set for exact bit length
    if (bits > 0) {
        bn_set_bit(bn, bits - 1, 1);
    }
}

void bn_random_range(BigNum* bn, const BigNum* min, const BigNum* max) {
    BigNum range, offset;
    bn_sub(&range, max, min);

    int bits = bn_bit_length(&range);
    do {
        bn_random(&offset, bits);
    } while (bn_compare(&offset, &range) >= 0);

    bn_add(bn, min, &offset);
}

void bn_gcd(BigNum* result, const BigNum* a, const BigNum* b) {
    BigNum x, y, temp;
    bn_copy(&x, a);
    bn_copy(&y, b);

    while (!bn_is_zero(&y)) {
        bn_mod(&temp, &x, &y);
        bn_copy(&x, &y);
        bn_copy(&y, &temp);
    }

    bn_copy(result, &x);
}

bool bn_is_prime_mr(const BigNum* n, int rounds, int mul_method) {
    // Handle small cases
    BigNum two, three;
    bn_init(&two);
    two.words[0] = 2;
    bn_init(&three);
    three.words[0] = 3;

    if (bn_compare(n, &two) < 0) return false;
    if (bn_compare(n, &two) == 0 || bn_compare(n, &three) == 0) return true;
    if (bn_is_even(n)) return false;

    // Factor out powers of 2: n - 1 = 2^s * d
    BigNum n_minus_1, d;
    bn_sub(&n_minus_1, n, &two);
    n_minus_1.words[0]++;  // n-1

    bn_copy(&d, &n_minus_1);
    int s = 0;
    while (bn_is_even(&d)) {
        bn_shift_right(&d, 1);
        s++;
    }

    // Perform rounds of testing
    for (int round = 0; round < rounds; round++) {
        BigNum a, x, n_minus_1_bn;
        bn_sub(&n_minus_1_bn, n, &two);
        n_minus_1_bn.words[0]++;

        // Pick random witness a in [2, n-2]
        bn_random_range(&a, &two, &n_minus_1_bn);

        // Compute x = a^d mod n
        bn_mod_pow(&x, &a, &d, n, mul_method);

        BigNum one;
        bn_init(&one);
        one.words[0] = 1;

        if (bn_compare(&x, &one) == 0 || bn_compare(&x, &n_minus_1_bn) == 0) {
            continue;
        }

        bool composite = true;
        for (int j = 0; j < s - 1; j++) {
            bn_mod_mul(&x, &x, &x, n, mul_method);

            if (bn_compare(&x, &n_minus_1_bn) == 0) {
                composite = false;
                break;
            }
        }

        if (composite) return false;
    }

    return true;
}

int bn_auto_select_mul_method(const BigNum* a, const BigNum* b) {
    int max_words = (a->num_words > b->num_words) ? a->num_words : b->num_words;
    int bits = max_words * WORD_BITS;

    if (bits < 1000) {
        return MUL_METHOD_STANDARD;
    } else if (bits < 8000) {
        return MUL_METHOD_KARATSUBA;
    } else {
        return MUL_METHOD_SCHONHAGE_STRASSEN;
    }
}

const char* bn_get_error_message(int error_code) {
    switch (error_code) {
        case BN_SUCCESS:
            return "Success";
        case BN_ERROR_OVERFLOW:
            return "Overflow error";
        case BN_ERROR_DIVISION_BY_ZERO:
            return "Division by zero";
        case BN_ERROR_INVALID_INPUT:
            return "Invalid input";
        default:
            return "Unknown error";
    }
}