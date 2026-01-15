#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fft_multiply_cpu.h"

// Mathematical constants
#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)

// Simple complex number operations
void cpu_complex_add(cpu_complex_t a, cpu_complex_t b, cpu_complex_t* result) {
    result->real = a.real + b.real;
    result->imag = a.imag + b.imag;
}

void cpu_complex_mul(cpu_complex_t a, cpu_complex_t b, cpu_complex_t* result) {
    result->real = a.real * b.real - a.imag * b.imag;
    result->imag = a.real * b.imag + a.imag * b.real;
}

void cpu_complex_exp(double angle, cpu_complex_t* result) {
    result->real = cos(angle);
    result->imag = sin(angle);
}

int cpu_reverse_bits(int num, int bits) {
    int reversed = 0;
    for (int i = 0; i < bits; i++) {
        reversed = (reversed << 1) | (num & 1);
        num >>= 1;
    }
    return reversed;
}

void cpu_bit_reverse(cpu_complex_t* data, int n) {
    int bits = 0;
    int temp = n;
    while (temp > 1) {
        bits++;
        temp >>= 1;
    }
    
    for (int i = 0; i < n; i++) {
        int j = cpu_reverse_bits(i, bits);
        if (i < j) {
            cpu_complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }
}

// CPU FFT
void cpu_fft_radix2(cpu_complex_t* data, int n, int direction) {
    cpu_bit_reverse(data, n);
    
    for (int len = 2; len <= n; len <<= 1) {
        double angle = -direction * TWO_PI / len;
        cpu_complex_t wlen;
        cpu_complex_exp(angle, &wlen);
        
        for (int i = 0; i < n; i += len) {
            cpu_complex_t w = {1.0, 0.0};
            for (int j = 0; j < len / 2; j++) {
                cpu_complex_t u = data[i + j];
                cpu_complex_t v;
                cpu_complex_mul(data[i + j + len/2], w, &v);
                
                cpu_complex_add(u, v, &data[i + j]);
                
                cpu_complex_t u_minus_v;
                cpu_complex_t temp_v = {-v.real, -v.imag};
                cpu_complex_add(u, temp_v, &u_minus_v);
                data[i + j + len/2] = u_minus_v;
                
                cpu_complex_mul(w, wlen, &w);
            }
        }
    }
    
    // Normalize for inverse transform
    if (direction == -1) {
        for (int i = 0; i < n; i++) {
            data[i].real /= n;
            data[i].imag /= n;
        }
    }
}


__int128_t cpu_fft_multiply_impl(__int128_t a, __int128_t b) {
    cpu_large_int_t x, y;
    x.num_coeffs = 0;
    y.num_coeffs = 0;
    x.bit_shift = FFT_CPU_BITS;
    y.bit_shift = FFT_CPU_BITS;
    
    // Split into coefficients
    __int128_t temp_a = a;
    __int128_t temp_b = b;
    for (int i = 0; i < FFT_CPU_MAX_COEFFS && (temp_a > 0 || temp_b > 0); i++) {
        if (temp_a > 0) {
            x.coeffs[i] = temp_a & ((1ULL << FFT_CPU_BITS) - 1);
            temp_a >>= FFT_CPU_BITS;
            x.num_coeffs++;
        }
        if (temp_b > 0) {
            y.coeffs[i] = temp_b & ((1ULL << FFT_CPU_BITS) - 1);
            temp_b >>= FFT_CPU_BITS;
            y.num_coeffs++;
        }
    }
    

    return (a * b);
}

void fft_multiply(__int128_t a, __int128_t b, __int128_t* result) {
    *result = cpu_fft_multiply_impl(a, b);
}