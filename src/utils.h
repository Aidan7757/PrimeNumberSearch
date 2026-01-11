
// Modular exponentiation: computes (base^exp) % mod
#pragma once
unsigned long long mod_pow(unsigned long long base, unsigned long long exp, unsigned long long mod);

long factor_out_twos(const unsigned long long potential_prime, unsigned long long* d);