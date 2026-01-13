#include <math.h>
#include <stdint.h>

#ifdef HAVE_CUDA
#include <cuda_runtime.h>
#endif

#include "schonhage_strassen.h"
#include "fft_multiply.h"

#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)

// Utility function to find next power of two
CUDA_DEVICE_FN int next_power_of_two(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

// Calculate root of unity for FFT
CUDA_DEVICE_FN double ss_root_of_unity(int n, int k, int direction) {
    double angle = direction * TWO_PI * k / n;
    return cos(angle);
}

// Split large integer into polynomial coefficients
CUDA_DEVICE_FN void ss_split_number(uint64_t num, polynomial_t* poly, int chunk_size) {
    poly->chunk_size = chunk_size;
    poly->num_coeffs = 0;
    
    uint64_t mask = (1ULL << chunk_size) - 1;
    
    while (num > 0 && poly->num_coeffs < SS_MAX_CHUNKS) {
        poly->coeffs[poly->num_coeffs] = (uint32_t)(num & mask);
        num >>= chunk_size;
        poly->num_coeffs++;
    }
}

// FFT implementation specifically for integer convolution
CUDA_DEVICE_FN void ss_fft(fft_workspace_t* ws, int direction) {
    int n = ws->size;
    
    // Bit reversal
    for (int i = 0; i < n; i++) {
        int j = i, bits = 0;
        int temp = n;
        while (temp > 1) {
            bits++;
            temp >>= 1;
        }
        
        int reversed = 0;
        for (int k = 0; k < bits; k++) {
            reversed = (reversed << 1) | (j & 1);
            j >>= 1;
        }
        
        if (i < reversed) {
            double temp_real = ws->real[i];
            double temp_imag = ws->imag[i];
            ws->real[i] = ws->real[reversed];
            ws->imag[i] = ws->imag[reversed];
            ws->real[reversed] = temp_real;
            ws->imag[reversed] = temp_imag;
        }
    }
    
    // Cooley-Tukey iterations
    for (int len = 2; len <= n; len <<= 1) {
        for (int i = 0; i < n; i += len) {
            for (int j = 0; j < len / 2; j++) {
                double angle = direction * TWO_PI * j / len;
                double cos_angle = cos(angle);
                double sin_angle = sin(angle);
                
                int idx1 = i + j;
                int idx2 = i + j + len / 2;
                
                double real1 = ws->real[idx1];
                double imag1 = ws->imag[idx1];
                double real2 = ws->real[idx2];
                double imag2 = ws->imag[idx2];
                
                double real_mul = real2 * cos_angle - imag2 * sin_angle;
                double imag_mul = real2 * sin_angle + imag2 * cos_angle;
                
                ws->real[idx1] = real1 + real_mul;
                ws->imag[idx1] = imag1 + imag_mul;
                ws->real[idx2] = real1 - real_mul;
                ws->imag[idx2] = imag1 - imag_mul;
            }
        }
    }
    
    // Normalization for inverse FFT
    if (direction == -1) {
        for (int i = 0; i < n; i++) {
            ws->real[i] /= n;
            ws->imag[i] /= n;
        }
    }
}

// Pointwise multiplication of FFT results
CUDA_DEVICE_FN void ss_pointwise_multiply(fft_workspace_t* a, fft_workspace_t* b, fft_workspace_t* result) {
    int n = a->size;
    for (int i = 0; i < n; i++) {
        double real_part = a->real[i] * b->real[i] - a->imag[i] * b->imag[i];
        double imag_part = a->real[i] * b->imag[i] + a->imag[i] * b->real[i];
        result->real[i] = real_part;
        result->imag[i] = imag_part;
    }
}

// Polynomial convolution using FFT
CUDA_DEVICE_FN void ss_convolution(polynomial_t* a, polynomial_t* b, polynomial_t* result) {
    fft_workspace_t ws_a, ws_b, ws_result;
    
    // Determine FFT size
    int n = next_power_of_two(a->num_coeffs + b->num_coeffs);
    ws_a.size = ws_b.size = ws_result.size = n;
    
    // Initialize FFT workspaces with polynomial coefficients
    for (int i = 0; i < n; i++) {
        if (i < a->num_coeffs) {
            ws_a.real[i] = (double)a->coeffs[i];
            ws_a.imag[i] = 0.0;
        } else {
            ws_a.real[i] = 0.0;
            ws_a.imag[i] = 0.0;
        }
        
        if (i < b->num_coeffs) {
            ws_b.real[i] = (double)b->coeffs[i];
            ws_b.imag[i] = 0.0;
        } else {
            ws_b.real[i] = 0.0;
            ws_b.imag[i] = 0.0;
        }
    }
    
    // Forward FFT
    ss_fft(&ws_a, 1);
    ss_fft(&ws_b, 1);
    
    // Pointwise multiplication
    ss_pointwise_multiply(&ws_a, &ws_b, &ws_result);
    
    // Inverse FFT
    ss_fft(&ws_result, -1);
    
    // Extract result coefficients with rounding
    result->num_coeffs = a->num_coeffs + b->num_coeffs;
    result->chunk_size = a->chunk_size;
    
    for (int i = 0; i < result->num_coeffs && i < SS_MAX_CHUNKS; i++) {
        // Round to nearest integer and handle overflow
        double rounded = round(ws_result.real[i]);
        if (rounded < 0) rounded = 0;
        if (rounded > 0xFFFFFFFF) rounded = 0xFFFFFFFF;
        
        result->coeffs[i] = (uint32_t)rounded;
    }
}

// Polynomial multiplication using FFT
CUDA_DEVICE_FN void ss_polynomial_mul(polynomial_t* a, polynomial_t* b, polynomial_t* result, fft_workspace_t* ws) {
    // For simplicity, we'll use the convolution function
    // In a full implementation, this would handle weighting and unweighting
    ss_convolution(a, b, result);
}

// Combine polynomial coefficients back into integer
CUDA_DEVICE_FN uint64_t ss_combine_coeffs(polynomial_t* poly, uint64_t mod) {
    uint64_t result = 0;
    uint64_t shift = 1;
    
    for (int i = 0; i < poly->num_coeffs; i++) {
        result += ((uint64_t)poly->coeffs[i] * shift) % mod;
        result %= mod;
        shift = (shift << poly->chunk_size) % mod;
    }
    
    return result;
}

// Main Schönhage-Strassen multiplication function
CUDA_DEVICE_FN uint64_t schonhage_strassen_mul(uint64_t a, uint64_t b, uint64_t mod) {
    // For numbers smaller than threshold, use traditional multiplication
    if (a < (1ULL << SS_MIN_BITS) && b < (1ULL << SS_MIN_BITS)) {
        uint64_t result = 0;
        a %= mod;
        
        while (b > 0) {
            if (b & 1) {
                result = (result + a) % mod;
            }
            a = (a + a) % mod;
            b >>= 1;
        }
        return result;
    }
    
    // Use Schönhage-Strassen for very large numbers
    polynomial_t poly_a, poly_b, poly_result;
    fft_workspace_t workspace;
    
    // Split numbers into polynomial coefficients
    ss_split_number(a, &poly_a, SS_BASE_CHUNK);
    ss_split_number(b, &poly_b, SS_BASE_CHUNK);
    
    // Initialize workspace
    int n = next_power_of_two(poly_a.num_coeffs + poly_b.num_coeffs);
    workspace.size = n;
    
    // Multiply polynomials using FFT
    ss_polynomial_mul(&poly_a, &poly_b, &poly_result, &workspace);
    
    // Combine coefficients back into integer modulo
    return ss_combine_coeffs(&poly_result, mod);
}

// CUDA kernel for batch Schönhage-Strassen multiplication
#ifdef HAVE_CUDA
__global__ void schonhage_strassen_kernel(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < count) {
        result[idx] = schonhage_strassen_mul(a[idx], b[idx], mod[idx]);
    }
}
#else
// CPU fallback
void schonhage_strassen_kernel(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = schonhage_strassen_mul(a[i], b[i], mod[i]);
    }
}
#endif

// Host wrapper function
void cuda_schonhage_strassen_multiply(const uint64_t* a, const uint64_t* b, const uint64_t* mod, uint64_t* result, int count) {
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
    schonhage_strassen_kernel<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_mod, d_result, count);
    
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
        result[i] = schonhage_strassen_mul(a[i], b[i], mod[i]);
    }
#endif
}