# CUDA Prime Number Search

This directory contains CUDA implementations for parallel prime number searching, optimized for GPU execution with massive parallelization over the outer loop that checks different numbers. The implementation includes fallback support for systems without CUDA.

## Files

- `src/search_methods_cuda.h` - CUDA header file with function declarations
- `src/search_methods_cuda.cu` - CUDA implementation with device functions and kernels
- `cuda_prime_search.c` - Main program to demonstrate CUDA usage
- `Makefile.cuda` - Makefile for building the CUDA program

## Prerequisites

### Minimum Requirements (CPU Fallback):
- GCC or compatible C compiler
- Make utility

### For GPU Acceleration:
1. **CUDA Toolkit**: Install NVIDIA CUDA Toolkit (version 7.0 or higher)
   - Download from: https://developer.nvidia.com/cuda-downloads
   - Ensure `nvcc` is in your PATH

2. **NVIDIA GPU**: CUDA-compatible GPU with compute capability 3.0 or higher

3. **Development Tools**:
   - GCC or compatible C compiler
   - Make utility

**Note**: The program will automatically detect CUDA availability and use GPU acceleration if present, otherwise it falls back to CPU implementation.

## Building

1. Check build configuration:
   ```bash
   make -f Makefile.cuda show-config
   ```

2. Check CUDA installation:
   ```bash
   make -f Makefile.cuda check-cuda
   ```

3. Build the program:
   ```bash
   make -f Makefile.cuda
   ```

4. Clean build artifacts:
   ```bash
   make -f Makefile.cuda clean
   ```

The build system automatically detects CUDA availability:
- **With CUDA**: Builds full GPU-accelerated version
- **Without CUDA**: Builds CPU fallback version (still functional!)

## Usage

```bash
./cuda_prime_search <method> <start> <end> <rounds>
```

### Parameters

- `method`: Primality test method (0-4)
  - 0: Naive check (deterministic)
  - 1: Miller-Rabin (probabilistic)
  - 2: Fermat (probabilistic)
  - 3: Gauss-Euler (deterministic)
  - 4: Miller-Rabin + Gauss-Euler (hybrid)

- `start`: Starting number (inclusive)
- `end`: Ending number (exclusive)
- `rounds`: Number of test rounds (for probabilistic methods)

### Examples

1. Search for primes using Miller-Rabin:
   ```bash
   ./cuda_prime_search 1 1000000 1000100 10
   ```

2. Search using naive method:
   ```bash
   ./cuda_prime_search 0 1000 2000 1
   ```

3. Large range search with Fermat test:
   ```bash
   ./cuda_prime_search 2 10000000 10010000 20
   ```

## Performance

The CUDA implementation provides massive parallelization by:

- Assigning each number to a separate GPU thread
- Using 256 threads per block for optimal GPU utilization
- Parallelizing the outer loop across thousands of GPU cores
- Keeping all primality testing logic on the GPU for minimal data transfer

### Expected Performance

- **Throughput**: Millions of numbers per second (depending on GPU and method)
- **Scalability**: Linear scaling with GPU compute capability
- **Memory Efficiency**: Minimal host-device data transfer

## CUDA Kernel Details

### Kernel Configuration
- **Threads per block**: 256 (configurable)
- **Blocks per grid**: Calculated based on input size
- **Memory allocation**: Device arrays for numbers and results

### Device Functions
All primality tests are implemented as `__device__` functions:
- `cuda_naive_check()`: Simple trial division
- `cuda_miller_rabin()`: Probabilistic Miller-Rabin test
- `cuda_fermat()`: Fermat's little theorem test
- `cuda_gauss_euler()`: Gauss-Euler quadratic residue test
- `cuda_mr_ge()`: Hybrid Miller-Rabin + Gauss-Euler

### Utility Functions
- `cuda_mod_mul()`: Overflow-safe modular multiplication
- `cuda_mod_pow()`: Modular exponentiation
- `cuda_factor_out_twos()`: Factor out powers of 2 for Miller-Rabin

## Integration with Existing Code

The CUDA implementation is designed to work alongside your existing OpenMP and MPI versions:

1. **Same Interface**: Uses the same primality test logic
2. **Compatible Data Types**: Works with `long long` integers
3. **Configurable**: Easy to switch between methods
4. **Independent**: Can be used separately or combined with other approaches

## Troubleshooting

### Common Issues

1. **"nvcc not found"**:
   - Install CUDA toolkit
   - Add CUDA bin directory to PATH
   - Update CUDA_PATH in Makefile if needed

2. **"CUDA runtime error"**:
   - Check GPU compatibility
   - Ensure NVIDIA drivers are up to date
   - Verify CUDA installation

3. **Compilation errors**:
   - Check CUDA architecture flag (`-arch=sm_XX`)
   - Ensure proper compute capability for your GPU
   - Update CUDA_ARCH in Makefile

### GPU Information

Check your GPU's compute capability:
```bash
nvidia-smi
```

Or use CUDA device query:
```bash
$(CUDA_PATH)/bin/deviceQuery
```

## Performance Tips

1. **Batch Processing**: Process large ranges for better GPU utilization
2. **Method Selection**: Use probabilistic methods for better performance
3. **Memory Alignment**: Ensure proper memory alignment for large arrays
4. **Async Transfers**: Use CUDA streams for overlapping computation and data transfer

## Example Output

```
Searching for primes in range [1000000, 1000100) using method 1 with 10 rounds...
Prime found: 1000003
Prime found: 1000033
Prime found: 1000037
Prime found: 1000039
Prime found: 1000099
Prime found: 1000091

Summary:
  Numbers checked: 100
  Primes found: 6
  Time elapsed: 0.0012 seconds
  Throughput: 83333 numbers/second
```

## License

This CUDA implementation follows the same license as the original Prime Number Search project.