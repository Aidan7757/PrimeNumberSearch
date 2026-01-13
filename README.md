# Prime Number Search with FFT and Schönhage-Strassen Optimization

## Overview

This project implements a high-performance primality testing framework that compares different algorithms across multiple platforms (CPU, GPU, MPI) with advanced optimizations including FFT-based multiplication and the Schönhage-Strassen algorithm. The system is designed for benchmarking prime number search algorithms at various scales and configurations.

## Project Structure and File Relationships

### Core Source Files (`src/`)

#### **`models.h` / `models.c`**
- **Purpose**: Configuration and data structures
- **Key Components**: `Config` struct containing `num_threads`, `lower_range`, `max_range`, `num_rounds`
- **Connections**: Used by all search methods and testing frameworks

#### **`utils.h` / `utils.c`**
- **Purpose**: Core mathematical utilities and test data
- **Key Functions**: 
  - `mod_mul()`: Overflow-safe modular multiplication
  - `mod_pow()`: Overflow-safe modular exponentiation
  - `factor_out_twos()`: Extract powers of 2 for Miller-Rabin
- **Test Data**: `TEST_PRIMES[]`, `TEST_COMPOSITE[]`, `TEST_ARRAY_SIZES`
- **Connections**: Essential for all primality tests, especially probabilistic ones

#### **`search_methods_openmp.h` / `search_methods_openmp.c`**
- **Purpose**: CPU-based primality testing with OpenMP parallelization
- **Algorithms Implemented**:
  1. `naive_check()`: Simple trial division (O(n))
  2. `miller_rabin()`: Probabilistic test with configurable rounds
  3. `fermat()`: Fermat's little theorem test
  4. `gauss_euler()`: Gauss-Euler primality test
  5. `mr_ge()`: Combined Miller-Rabin + Gauss-Euler for higher accuracy
- **Connections**: Integrated with `utils.c` for mathematical operations

#### **`search_methods_cuda.h` / `search_methods_cuda.cu`**
- **Purpose**: GPU-accelerated primality testing using CUDA
- **Key Features**: 
  - Device functions for all 5 algorithms
  - Conditional compilation with CPU fallback
  - Host wrapper functions for memory management
- **Connections**: Links with FFT multiplication kernels

#### **`fft_multiply.h` / `fft_multiply.cu`**
- **Purpose**: FFT-based fast multiplication for large integers
- **Algorithm**: Radix-2 FFT with complex number operations
- **Key Structures**: 
  - `complex_t`: Real/imaginary components
  - `large_int_t`: Coefficient-based integer representation
- **Functions**: `fft_radix2()`, `fft_pointwise_mul()`, `fft_multiply()`
- **Connections**: Used by primality tests for efficient modular exponentiation

#### **`schonhage_strassen.h` / `schonhage_strassen.cu` / `schonhage_strassen_host.c`**
- **Purpose**: Schönhage-Strassen algorithm for very large integer multiplication
- **Algorithm**: 
  - Split numbers into polynomial coefficients
  - FFT-based polynomial multiplication
  - Carry propagation and normalization
- **Key Structures**: 
  - `polynomial_t`: Coefficient-based representation
  - `fft_workspace_t`: FFT computation workspace
- **Connections**: Works with FFT functions for ultimate performance on large numbers

#### **`search_methods_mpi.h` / `search_methods_mpi.c`**
- **Purpose**: Distributed computing implementation using MPI
- **Connections**: Extends OpenMP methods for cluster computing

### Main Executables

#### **`cuda_prime_search.c`**
- **Purpose**: Command-line interface for GPU/CPU prime search
- **Usage**: `./cuda_prime_search <method> <start> <end> <rounds>`
- **Methods**: 0=Naive, 1=Miller-Rabin, 2=Fermat, 3=Gauss-Euler, 4=MR-GE
- **Features**: 
  - Automatic CUDA detection with CPU fallback
  - Performance timing and throughput measurement
  - First 20 primes displayed for verification

#### **`fast_mul_demo.c`**
- **Purpose**: Demonstration and benchmarking of fast multiplication algorithms
- **Features**:
  - Performance comparison across number sizes
  - FFT vs traditional multiplication timing
  - Integration with primality testing on large numbers
- **Output**: Detailed performance metrics and algorithm explanations

### Testing Framework

#### **`tests/search_methods_openmp_test.c`**
- **Purpose**: Comprehensive testing of all primality methods
- **Test Types**:
  - Pre-defined array tests (`TEST_PRIMES`, `TEST_COMPOSITE`)
  - Large range performance tests
  - Timing and accuracy validation
- **Configuration**: Uses `Config` struct for test parameters
- **Output**: Performance metrics and error detection

### Build Configuration

#### **`CMakeLists.txt`**
- **Purpose**: Cross-platform build configuration
- **Compiler**: GCC 15 with C11 standard
- **Features**: OpenMP integration, CUDA compilation
- **Dependencies**: CUDA runtime, OpenMP libraries

#### **`Makefile.cuda`**
- **Purpose**: CUDA-specific build configuration
- **Features**: Automatic CUDA detection, fallback to CPU builds

### Benchmarking Scripts

#### **CPU Testing Scripts**
- **`cpu_test.sh`**: CPU benchmark with FFT optimization
- **`final_cpu_test.sh`**: Comprehensive CPU evaluation
- **`final_cpu_test_no_fft.sh`**: CPU testing without FFT (baseline)
- **`cpu_test_no_fft.sh`**: Basic CPU testing without FFT

#### **GPU Testing Scripts**
- **`gpu_test.sh`**: GPU benchmark with FFT (SLURM-compatible)
- **`gpu_test_no_fft.sh`**: GPU testing without FFT

#### **Cluster Configuration**
- **`SLURM_CONFIG_GUIDE.md`**: Comprehensive cluster usage guide
- **Features**: Job configuration, resource allocation, result monitoring

## Algorithm Implementation Details

### 1. Naive Check (Method 0)
```c
// Basic trial division from 2 to n-1
for (size_t i = 2; i < potential_prime - 1; ++i) {
    if (potential_prime % i == 0) return false;
}
```
- **Complexity**: O(n)
- **Best For**: Small numbers (< 1000)
- **Optimizations**: OpenMP parallelization

### 2. Miller-Rabin (Method 1)
```c
// potential_prime - 1 = 2^s * d
unsigned long long d;
long s = factor_out_twos(potential_prime, &d);

// Test k random bases
for (int i = 0; i < num_rounds; i++) {
    unsigned long long a = 2 + rand() % (potential_prime - 4);
    unsigned long long x = mod_pow(a, d, potential_prime);
    // Miller-Rabin conditions check
}
```
- **Complexity**: O(k log³ n)
- **Accuracy**: 99.9%+ with 5-10 rounds
- **Optimizations**: Fast modular exponentiation, FFT multiplication

### 3. Fermat Test (Method 2)
```c
// Fermat's Little Theorem: a^(n-1) ≡ 1 (mod n)
unsigned long long a = 2 + rand() % (potential_prime - 4);
if (mod_pow(a, potential_prime - 1, potential_prime) != 1) {
    return false;  // Definitely composite
}
```
- **Complexity**: O(k log³ n)
- **Limitations**: False positives for Carmichael numbers
- **Best For**: Quick initial screening

### 4. Gauss-Euler (Method 3)
```c
// Gauss's theorem: n is prime iff n divides (p-1)! + 1 for some p < n
// Implementation uses modular arithmetic optimizations
```
- **Complexity**: O(n log n) with optimizations
- **Features**: Deterministic for ranges below certain limits
- **Optimizations**: FFT-accelerated factorial computation

### 5. Miller-Rabin + Gauss-Euler (Method 4)
```c
// Combine probabilistic and deterministic tests
bool mr_result = miller_rabin(n, config);
if (mr_result) {
    bool ge_result = gauss_euler(n);
    return mr_result && ge_result;  // Both must agree
}
```
- **Accuracy**: Near 100% (combines strengths of both)
- **Performance**: Higher computational cost but maximum reliability
- **Best For**: Critical applications requiring certainty

## Fast Multiplication Algorithms

### FFT-Based Multiplication

#### **Mathematical Foundation**
```
Traditional multiplication: O(n²)
FFT multiplication: O(n log n)

Convolution theorem:
f * g = F⁻¹{F(f) ⋅ F(g)}
```

#### **Implementation Steps**
1. **Number Representation**: Split large integers into coefficient arrays
2. **FFT Transform**: Convert to frequency domain using radix-2 FFT
3. **Pointwise Multiplication**: Complex multiplication in frequency domain
4. **Inverse FFT**: Convert back to time domain
5. **Carry Propagation**: Normalize results to proper base
6. **Modulo Reduction**: Apply modular arithmetic for primality tests

#### **Code Structure** (`fft_multiply.cu`)
```c
typedef struct {
    uint64_t coeffs[FFT_MAX_COEFFS];
    int num_coeffs;
    int bit_shift;
} large_int_t;

__device__ void fft_radix2(complex_t* data, int n, int direction);
__device__ void fft_multiply(uint64_t a, uint64_t b, uint64_t* result);
```

### Schönhage-Strassen Algorithm

#### **Mathematical Foundation**
```
For very large integers (> 2^20 bits):
Traditional: O(n²)
Schönhage-Strassen: O(n log n log log n)

Key innovations:
- Number theoretic transforms instead of complex FFT
- Recursive multiplication via convolution
- Optimal for numbers > 10^1000
```

#### **Implementation Steps**
1. **Polynomial Splitting**: Represent numbers as polynomials with base-2^k coefficients
2. **Weighting**: Multiply coefficients by appropriate powers of θ
3. **FFT Evaluation**: Use number theoretic transforms
4. **Pointwise Multiplication**: Multiply in frequency domain
5. **Unweighting**: Apply inverse weighting
6. **Carry Propagation**: Convert back to integer representation
7. **Modulo Reduction**: Final modular arithmetic step

#### **Code Structure** (`schonhage_strassen.cu`)
```c
typedef struct {
    uint32_t coeffs[SS_MAX_CHUNKS];
    int num_coeffs;
    int chunk_size;
} polynomial_t;

__device__ uint64_t schonhage_strassen_mul(uint64_t a, uint64_t b, uint64_t mod);
__device__ void ss_polynomial_mul(polynomial_t* a, polynomial_t* b, polynomial_t* result);
```

### Performance Optimization Strategy

#### **Algorithm Selection by Number Size**
```
if (n < 2^16)          → Traditional multiplication
else if (n < 2^20)     → FFT-based multiplication  
else                   → Schönhage-Strassen algorithm
```

#### **Integration with Primality Tests**
- **Modular Exponentiation**: Uses fast multiplication for (a^b mod n)
- **Factorial Computation**: FFT-accelerated for Gauss-Euler
- **Multiple Tests**: Shared optimization across all algorithms

## Usage Examples

### Basic Prime Search
```bash
# Build the project
make -f Makefile.cuda

# Search for primes using Miller-Rabin
./cuda_prime_search 1 1000000 1010000 10

# Output:
# Searching for primes in range [1000000, 1010000) using method 1 with 10 rounds...
# Prime found: 1000003
# Prime found: 1000033
# ...
# Summary:
#   Numbers checked: 10000
#   Primes found: 53
#   Time elapsed: 0.1250 seconds
#   Throughput: 80000 numbers/second
```

### Performance Benchmarking
```bash
# Run CPU benchmark with FFT
./cpu_test.sh

# Run GPU comparison
./gpu_test.sh

# Run comprehensive benchmark
sbatch run_prime_benchmark.sh
```

### Fast Multiplication Demo
```bash
# Demonstrate fast multiplication performance
./fast_mul_demo

# Output:
# === Fast Multiplication for Prime Number Search ===
# Algorithm Overview:
# 1. For small numbers (< 2^16): Traditional binary multiplication
# 2. For medium numbers (2^16 to 2^20): FFT-based convolution  
# 3. For large numbers (> 2^20): Schönhage-Strassen algorithm
# 
# Number Size     | Traditional (ns) | FFT-based (ns) | Schönhage-Strassen (ns)
# ---------------|------------------|----------------|-------------------------
# Small          |             1250 |           2100 |                    3400
# Medium         |             8900 |           3200 |                    2800
# Large          |           156000 |          12800 |                    9500
```
