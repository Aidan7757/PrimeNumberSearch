#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "schonhage_strassen_cpu.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FFT-based Schönhage-Strassen implementation
// Split numbers into polynomial coefficients, apply FFT, then reconstruct

// Split number into polynomial coefficients
static void cpu_ss_split_number(uint64_t num, cpu_polynomial_t* poly) {
    memset(poly->coeffs, 0, sizeof(poly->coeffs));
    poly->num_coeffs = 0;
    poly->chunk_size = SS_CPU_BASE_CHUNK;
    
    while (num > 0 && poly->num_coeffs < SS_CPU_MAX_CHUNKS) {
        poly->coeffs[poly->num_coeffs] = num & ((1ULL << poly->chunk_size) - 1);
        num >>= poly->chunk_size;
        poly->num_coeffs++;
    }
}

// Simple FFT implementation for Schönhage-Strassen
static void cpu_ss_fft(cpu_fft_workspace_t* ws, int direction) {
    int n = ws->size;
    
    // Bit-reversal permutation
    for (int i = 0; i < n; i++) {
        int j = 0;
        for (int k = 0; (1 << k) < n; k++) {
            if (i & (1 << k)) {
                j |= (1 << ((int)log2(n) - 1 - k));
            }
        }
        if (j > i) {
            double temp_real = ws->real[i];
            double temp_imag = ws->imag[i];
            ws->real[i] = ws->real[j];
            ws->imag[i] = ws->imag[j];
            ws->real[j] = temp_real;
            ws->imag[j] = temp_imag;
        }
    }
    
    // FFT stages
    for (int len = 2; len <= n; len <<= 1) {
        double angle = direction * M_PI / (len >> 1);
        
        for (int i = 0; i < n; i += len) {
            double w_real = 1.0;
            double w_imag = 0.0;
            
            for (int j = 0; j < len / 2; j++) {
                int u = i + j;
                int v = i + j + len / 2;
                
                double u_real = ws->real[u];
                double u_imag = ws->imag[u];
                double v_real = ws->real[v];
                double v_imag = ws->imag[v];
                
                // Butterfly operation
                ws->real[u] = u_real + v_real;
                ws->imag[u] = u_imag + v_imag;
                
                ws->real[v] = u_real - v_real;
                ws->imag[v] = u_imag - v_imag;
                
                // Apply twiddle factor
                v_real = ws->real[v];
                v_imag = ws->imag[v];
                ws->real[v] = v_real * w_real - v_imag * w_imag;
                ws->imag[v] = v_real * w_imag + v_imag * w_real;
                
                // Update twiddle factor
                double next_w_real = w_real * cos(angle) - w_imag * sin(angle);
                double next_w_imag = w_real * sin(angle) + w_imag * cos(angle);
                w_real = next_w_real;
                w_imag = next_w_imag;
            }
        }
    }
    
    // Scale for inverse FFT
    if (direction == -1) {
        for (int i = 0; i < n; i++) {
            ws->real[i] /= n;
            ws->imag[i] /= n;
        }
    }
}

// Convolution using FFT
static void ss_convolution(cpu_fft_workspace_t* a, cpu_fft_workspace_t* b, cpu_fft_workspace_t* result) {
    // Forward FFT
    cpu_ss_fft(a, 1);
    cpu_ss_fft(b, 1);
    
    // Pointwise multiplication
    for (int i = 0; i < a->size; i++) {
        double temp_real = a->real[i] * b->real[i] - a->imag[i] * b->imag[i];
        double temp_imag = a->real[i] * b->imag[i] + a->imag[i] * b->real[i];
        result->real[i] = temp_real;
        result->imag[i] = temp_imag;
    }
    
    // Inverse FFT
    cpu_ss_fft(result, -1);
}

// Pointwise multiplication for convolution
static void ss_pointwise_multiply(cpu_polynomial_t* a, cpu_polynomial_t* b, cpu_polynomial_t* result) {
    cpu_fft_workspace_t ws_a, ws_b, ws_result;
    
    ws_a.size = SS_CPU_MAX_CHUNKS * 2;
    ws_b.size = SS_CPU_MAX_CHUNKS * 2;
    ws_result.size = SS_CPU_MAX_CHUNKS * 2;
    
    // Load polynomials into FFT workspace
    memset(&ws_a, 0, sizeof(ws_a));
    memset(&ws_b, 0, sizeof(ws_b));
    memset(&ws_result, 0, sizeof(ws_result));
    
    for (int i = 0; i < a->num_coeffs; i++) {
        ws_a.real[i] = (double)a->coeffs[i];
    }
    for (int i = 0; i < b->num_coeffs; i++) {
        ws_b.real[i] = (double)b->coeffs[i];
    }
    
    // Perform convolution
    ss_convolution(&ws_a, &ws_b, &ws_result);
    
    // Extract results with carry propagation
    memset(result->coeffs, 0, sizeof(result->coeffs));
    result->num_coeffs = a->num_coeffs + b->num_coeffs;
    result->chunk_size = a->chunk_size;
    
    uint64_t carry = 0;
    for (int i = 0; i < result->num_coeffs; i++) {
        uint64_t value = (uint64_t)round(ws_result.real[i]) + carry;
        result->coeffs[i] = value & ((1ULL << result->chunk_size) - 1);
        carry = value >> result->chunk_size;
    }
    
    // Handle final carry
    while (carry > 0 && result->num_coeffs < SS_CPU_MAX_CHUNKS) {
        result->coeffs[result->num_coeffs] = carry & ((1ULL << result->chunk_size) - 1);
        carry >>= result->chunk_size;
        result->num_coeffs++;
    }
}

// Polynomial multiplication using Schönhage-Strassen
static void ss_polynomial_mul(cpu_polynomial_t* a, cpu_polynomial_t* b, cpu_polynomial_t* result) {
    ss_pointwise_multiply(a, b, result);
}

// Main Schönhage-Strassen multiplication implementation
static uint64_t cpu_schonhage_strassen_mul_impl(uint64_t a, uint64_t b, uint64_t mod) {
    // For demonstration with 64-bit numbers
    // In practice, SS algorithm is most efficient for very large numbers (> 2^16 bits)
    // This implementation shows the algorithm structure
    
    cpu_polynomial_t poly_a, poly_b, result;
    
    // Split numbers into polynomials
    cpu_ss_split_number(a, &poly_a);
    cpu_ss_split_number(b, &poly_b);
    
    // Perform polynomial multiplication using FFT convolution
    ss_polynomial_mul(&poly_a, &poly_b, &result);
    
    // Reconstruct the product
    uint64_t product = 0;
    for (int i = 0; i < result.num_coeffs; i++) {
        product += ((uint64_t)result.coeffs[i]) << (i * result.chunk_size);
    }
    
    // Apply modulo
    if (mod != 0) {
        product %= mod;
    }
    
    return product;
}

// Wrapper that matches CUDA interface
uint64_t schonhage_strassen_mul(uint64_t a, uint64_t b, uint64_t mod) {
    return cpu_schonhage_strassen_mul_impl(a, b, mod);
}