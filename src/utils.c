// Modular exponentiation: computes (base^exp) % mod
unsigned long long mod_pow(unsigned long long base, unsigned long long exp, unsigned long long mod) {
    unsigned long long res = 1;
    base %= mod;

    while (exp > 0) {
        if (exp & 1) {
            res = (res * base) % mod;
        }
        base = (base * base) % mod;
        exp >>= 1;
    }
    return res;
}

long factor_out_twos(const unsigned long long potential_prime, unsigned long long* d) {
    unsigned long long value = potential_prime - 1;
    long s = 0;

    while (value % 2 == 0) {
        value /= 2;
        s += 1;
    }
    *d = value;
    return s;
}