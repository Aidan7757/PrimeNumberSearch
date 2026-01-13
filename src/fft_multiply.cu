#include <math.h>
#include <stdint.h>

#ifdef HAVE_CUDA
#include <cuda_runtime.h>
#endif

#include "fft_multiply.h"

// Mathematical constants
#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)

// Complex number operations
CUDA_DEVICE_FN complex_t complex_add(complex_t a, complex_t b) {
    complex_t result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

CUDA_DEVICE_FN complex_t complex_sub(complex_t a, complex_t b) {
    complex_t result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

CUDA_DEVICE_FN complex_t complex_mul(complex_t a, complex_t b) {
    complex_t result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

CUDA_DEVICE_FN complex_t complex_exp(double angle) {
    complex_t result;
    result.real = cos(angle);
    result.imag = sin(angle);
    return result;
}

// Bit reversal for FFT
CUDA_DEVICE_FN int reverse_bits(int num, int bits) {
    int reversed = 0;
    for (int i = 0; i < bits; i++) {
        reversed = (reversed << 1) | (num & 1);
        num >>= 1;
    }
    return reversed;
}

CUDA_DEVICE_FN void bit_reverse(complex_t* data, int n) {
    int bits = 0;
    int temp = n;
    while (temp > 1) {
        bits++;
        temp >>= 1;
    }
    
    for (int i = 0; i < n; i++) {
        int reversed = reverse_bits(i, bits);
        if (i < reversed) {
            complex_t temp = data[i];
            data[i] = data[reversed];
            data[reversed] = temp;
        }
    }
}

// Cooley-Tukey FFT implementation (Radix-2)
CUDA_DEVICE_FN void fft_radix2(complex_t* data, int n, int direction) {
    // Bit reversal permutation
    bit_reverse(data, n);
    
    // Danielson-Lanczos section
    for (int len = 2; len <= n; len <<= 1) {
        double angle = -direction * TWO_PI / len;
        complex_t w_len = complex_exp(angle);
        
        for (int i = 0; i < n; i += len) {
            complex_t w = {1.0, 0.0};
            
            for (int j = 0; j < len / 2; j++) {
                complex_t u = data[i + j];
                complex_t v = complex_mul(w, data[i + j + len / 2]);
                
                data[i + j] = complex_add(u, v);
                data[i + j + len / 2] = complex_sub(u, v);
                
                w = complex_mul(w, w_len);
            }
        }
    }
    
    // Normalization for inverse transform
    if (direction == -1) {
        for (int i = 0; i < n; i++) {
            data[i].real /= n;
            data[i].imag /= n;
        }
    }
}

// Pointwise multiplication of FFT results
CUDA_DEVICE_FN void fft_pointwise_mul(complex_t* a, complex_t* b, complex_t* result, int n) {
    for (int i = 0; i < n; i++) {
        result[i] = complex_mul(a[i], b[i]);
    }
}

// Initialize large integer from 64-bit value
CUDA_DEVICE_FN void large_int_init(large_int_t* num, uint64_t value) {
    num->bit_shift = FFT_BITS;
    num->num_coeffs = 0;
    
    // Split 64-bit value into smaller coefficients
    uint64_t mask = (1ULL << FFT_BITS) - 1;
    int coeff_idx = 0;
    
    while (value > 0 && coeff_idx < FFT_MAX_COEFFS) {
        num->coeffs[coeff_idx] = value & mask;
        value >>= FFT_BITS;
        coeff_idx++;
    }
    
    num->num_coeffs = coeff_idx;
}

// Split large integer into FFT coefficients
CUDA_DEVICE_FN void large_int_split_coeffs(large_int_t* num) {
    // This is a simplified version - in practice, you'd need more sophisticated
    // coefficient splitting for very large numbers
    uint64_t mask = (1ULL << num->bit_shift) - 1;
    
    for (int i = 0; i < num->num_coeffs; i++) {
        num->coeffs[i] &= mask;
    }
}

// Reconstruct large integer from FFT coefficients
CUDA_DEVICE_FN void large_int_from_coeffs(large_int_t* num, uint64_t* result) {
    uint64_t value = 0;
    uint64_t shift = 1;
    
    for (int i = 0; i < num->num_coeffs; i++) {
        value += num->coeffs[i] * shift;
        shift <<= num->bit_shift;
    }
    
    *result = value;
}

// FFT-based multiplication for 64-bit integers
CUDA_DEVICE_FN void fft_multiply(uint64_t a, uint64_t b, uint64_t* result) {
    // For 64-bit numbers, direct multiplication is still faster
    // This is mainly a demonstration of the FFT approach
    // In practice, FFT multiplication becomes beneficial for >~1000-bit numbers
    
    large_int_t x, y;
    complex_t fft_a[FFT_MAX_COEFFS * 2], fft_b[FFT_MAX_COEFFS * 2], fft_result[FFT_MAX_COEFFS * 2];
    
    // Initialize large integers
    large_int_init(&x, a);
    large_int_init(&y, b);
    
    // Pad to power of 2 for FFT
    int n = 1;
    while (n < (x.num_coeffs + y.num_coeffs)) {
        n <<= 1;
    }
    
    // Convert to complex arrays
    for (int i = 0; i < n; i++) {
        if (i < x.num_coeffs) {
            fft_a[i].real = (double)x.coeffs[i];
            fft_a[i].imag = 0.0;
        } else {
            fft_a[i].real = 0.0;
            fft_a[i].imag = 0.0;
        }
        
        if (i < y.num_coeffs) {
            fft_b[i].real = (double)y.coeffs[i];
            fft_b[i].imag = 0.0;
        } else {
            fft_b[i].real = 0.0;
            fft_b[i].imag = 0.0;
        }
    }
    
    // Perform FFT
    fft_radix2(fft_a, n, 1);  // Forward FFT
    fft_radix2(fft_b, n, 1);  // Forward FFT
    
    // Pointwise multiplication
    fft_pointwise_mul(fft_a, fft_b, fft_result, n);
    
    // Inverse FFT
    fft_radix2(fft_result, n, -1);
    
    // Convert back to integer (simplified)
    large_int_t product;
    product.num_coeffs = x.num_coeffs + y.num_coeffs;
    product.bit_shift = x.bit_shift;
    
    for (int i = 0; i < product.num_coeffs && i < FFT_MAX_COEFFS; i++) {
        product.coeffs[i] = (uint64_t)round(fft_result[i].real);
    }
    
    large_int_from_coeffs(&product, result);
}

// Fast modular multiplication using FFT
CUDA_DEVICE_FN uint64_t cuda_fast_mod_mul(uint64_t a, uint64_t b, uint64_t mod) {
    // Use FFT multiplication for very large numbers
    // For 64-bit numbers, traditional method is still faster
    // This shows how it would work for larger numbers
    
    if (a == 0 || b == 0) return 0;
    
    uint64_t product;
    fft_multiply(a, b, &product);
    
    // Simple modulo reduction (can be optimized with Barrett reduction)
    return product % mod;
}

// CUDA kernel for batch FFT multiplication
#ifdef HAVE_CUDA
CUDA_GLOBAL_FN void fft_multiply_kernel(const uint64_t* a, const uint64_t* b, uint64_t* result, int count) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < count) {
        fft_multiply(a[idx], b[idx], &result[idx]);
    }
}

CUDA_GLOBAL_FN void fast_mod_mul_kernel(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < count) {
        result[idx] = cuda_fast_mod_mul(a[idx], b[idx], mod[idx]);
    }
}
#else
// Fallback CPU implementations
CUDA_GLOBAL_FN void fft_multiply_kernel(const uint64_t* a, const uint64_t* b, uint64_t* result, int count) {
    for (int i = 0; i < count; i++) {
        fft_multiply(a[i], b[i], &result[i]);
    }
}

CUDA_GLOBAL_FN void fast_mod_mul_kernel(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = cuda_fast_mod_mul(a[i], b[i], mod[i]);
    }
}
#endif

// Host wrapper functions
void cuda_fft_multiply(const uint64_t* a, const uint64_t* b, uint64_t* result, int count) {
#ifdef HAVE_CUDA
    uint64_t* d_a;
    uint64_t* d_b;
    uint64_t* d_result;
    
    // Allocate device memory
    cudaMalloc(&d_a, count * sizeof(uint64_t));
    cudaMalloc(&d_b, count * sizeof(uint64_t));
    cudaMalloc(&d_result, count * sizeof(uint64_t));
    
    // Copy data to device
    cudaMemcpy(d_a, a, count * sizeof(uint64_t), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, count * sizeof(uint64_t), cudaMemcpyHostToDevice);
    
    // Configure kernel launch
    int threadsPerBlock = 256;
    int blocksPerGrid = (count + threadsPerBlock - 1) / threadsPerBlock;
    
    // Launch kernel
    fft_multiply_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_result, count);
    
    // Copy results back
    cudaMemcpy(result, d_result, count * sizeof(uint64_t), cudaMemcpyDeviceToHost);
    
    // Free device memory
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_result);
#else
    // CPU fallback
    for (int i = 0; i < count; i++) {
        fft_multiply(a[i], b[i], &result[i]);
    }
#endif
}

void cuda_fast_mod_multiply(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count) {
#ifdef HAVE_CUDA
    uint64_t* d_a;
    uint64_t* d_b;
    uint64_t* d_mod;
    uint64_t* d_result;
    
    // Allocate device memory
    cudaMalloc(&d_a, count * sizeof(uint64_t));
    cudaMalloc(&d_b, count * sizeof(uint64_t));
    cudaMalloc(&d_mod, count * sizeof(uint64_t));
    cudaMalloc(&d_result, count * sizeof(uint64_t));
    
    // Copy data to device
    cudaMemcpy(d_a, a, count * sizeof(uint64_t), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, count * sizeof(uint64_t), cudaMemcpyHostToDevice);
    cudaMemcpy(d_mod, mod, count * sizeof(uint64_t), cudaMemcpyHostToDevice);
    
    // Configure kernel launch
    int threadsPerBlock = 256;
    int blocksPerGrid = (count + threadsPerBlock - 1) / threadsPerBlock;
    
    // Launch kernel
    fast_mod_mul_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_mod, d_result, count);
    
    // Copy results back
    cudaMemcpy(result, d_result, count * sizeof(uint64_t), cudaMemcpyDeviceToHost);
    
    // Free device memory
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_mod);
    cudaFree(d_result);
#else
    // CPU fallback
    for (int i = 0; i < count; i++) {
        result[i] = cuda_fast_mod_mul(a[i], b[i], mod[i]);
    }
#endif
}