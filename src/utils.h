
// Modular exponentiation: computes (base^exp) % mod
#pragma once
const extern int TEST_PRIMES[];
const extern int TEST_COMPOSITE[];
const extern size_t TEST_ARRAY_SIZES;

unsigned long long mod_mul(unsigned long long a, unsigned long long b, unsigned long long mod);

unsigned long long mod_pow(unsigned long long base, unsigned long long exp, unsigned long long mod);

unsigned long long non_mod_pow(unsigned long long base, unsigned long long exp);

long factor_out_twos(unsigned long long potential_prime, unsigned long long* d);