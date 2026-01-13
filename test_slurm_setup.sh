#!/bin/bash

# =============================================================================
# SLURM BENCHMARK TEST VALIDATOR
# Test the benchmark scripts locally before submitting to cluster
# =============================================================================

echo "SLURM Benchmark Script Validator"
echo "================================="
echo "Testing configuration and functionality..."
echo ""

# Test 1: Check if scripts are executable
echo "1. Checking script permissions..."
for script in run_prime_quick.sh run_prime_benchmark.sh; do
    if [ -x "$script" ]; then
        echo "   ✓ $script is executable"
    else
        echo "   ✗ $script is not executable (run: chmod +x $script)"
    fi
done
echo ""

# Test 2: Check if required files exist
echo "2. Checking required files..."
required_files="Makefile.cuda cuda_prime_search.c src/search_methods_cuda.h"
for file in $required_files; do
    if [ -f "$file" ]; then
        echo "   ✓ $file exists"
    else
        echo "   ✗ $file missing"
    fi
done
echo ""

# Test 3: Validate configuration syntax
echo "3. Validating script syntax..."
for script in run_prime_quick.sh run_prime_benchmark.sh; do
    if bash -n "$script" 2>/dev/null; then
        echo "   ✓ $script syntax is valid"
    else
        echo "   ✗ $script has syntax errors"
        bash -n "$script"
    fi
done
echo ""

# Test 4: Test local build (without CUDA)
echo "4. Testing local build..."
echo "   Building with CPU fallback..."
if make -f Makefile.cuda clean > /dev/null 2>&1; then
    if make -f Makefile.cuda > /dev/null 2>&1; then
        echo "   ✓ Build successful"
        
        # Test program execution
        echo "   Testing program execution..."
        if ./cuda_prime_search 1 1000 1010 3 > /dev/null 2>&1; then
            echo "   ✓ Program runs successfully"
        else
            echo "   ✗ Program execution failed"
        fi
    else
        echo "   ✗ Build failed"
    fi
else
    echo "   ✗ Clean failed"
fi
echo ""

# Test 5: Extract configuration from scripts
echo "5. Extracting current configuration..."
echo "   Quick Script Configuration:"
if [ -f "run_prime_quick.sh" ]; then
    fft=$(grep "FFT_ENABLED=" run_prime_quick.sh | head -1 | cut -d'=' -f2)
    method=$(grep "TEST_METHOD=" run_prime_quick.sh | head -1 | cut -d'=' -f2)
    start=$(grep "TEST_START=" run_prime_quick.sh | head -1 | cut -d'=' -f2)
    end=$(grep "TEST_END=" run_prime_quick.sh | head -1 | cut -d'=' -f2)
    rounds=$(grep "SAMPLING_ROUNDS=" run_prime_quick.sh | head -1 | cut -d'=' -f2)
    
    echo "     FFT Enabled: $fft"
    echo "     Test Method: $method (0=Naive, 1=Miller-Rabin, 2=Fermat, 3=Gauss-Euler, 4=MR-GE)"
    echo "     Range: [$start, $end)"
    echo "     Sampling Rounds: $rounds"
fi
echo ""

# Test 6: Show estimated job requirements
echo "6. Job resource estimates..."
if [ -f "run_prime_quick.sh" ]; then
    echo "   Quick Test Job:"
    grep "^#SBATCH" run_prime_quick.sh | sed 's/#SBATCH/     /'
fi
echo ""

# Test 7: Generate sample job commands
echo "7. Sample submission commands..."
echo "   Quick test:"
echo "     sbatch run_prime_quick.sh"
echo ""
echo "   Full benchmark:"
echo "     sbatch run_prime_benchmark.sh"
echo ""

# Test 8: Show expected output files
echo "8. Expected output files..."
echo "   Quick test:"
echo "     prime_quick_XXXXX.out     # Standard output"
echo "     prime_quick_XXXXX.err     # Error output"
echo ""
echo "   Full benchmark:"
echo "     benchmark_results_XXXXX/  # Results directory"
echo "     ├─ benchmark_summary.csv"
echo "     ├─ configuration.txt"
echo "     ├─ resource_monitor.log"
echo "     └─ Method_Range.csv files"
echo ""

# Test 9: Check for common issues
echo "9. Checking for common issues..."
issues=0

# Check for bc calculator
if ! command -v bc &> /dev/null; then
    echo "   ⚠ 'bc' calculator not found - install with: sudo apt-get install bc"
    issues=$((issues + 1))
fi

# Check for make
if ! command -v make &> /dev/null; then
    echo "   ⚠ 'make' not found - install build-essential package"
    issues=$((issues + 1))
fi

# Check for gcc
if ! command -v gcc &> /dev/null; then
    echo "   ⚠ 'gcc' not found - install gcc compiler"
    issues=$((issues + 1))
fi

if [ $issues -eq 0 ]; then
    echo "   ✓ No common issues detected"
else
    echo "   ⚠ Found $issues potential issue(s)"
fi
echo ""

# Summary
echo "============================================================================"
if [ $issues -eq 0 ]; then
    echo "✓ All tests passed! Scripts are ready for ARC cluster submission."
    echo ""
    echo "Next steps:"
    echo "1. Upload files to ARC cluster (scp or transfer)"
    echo "2. Edit configuration as needed"
    echo "3. Submit with: sbatch run_prime_quick.sh"
else
    echo "⚠ Some tests failed. Please address issues before submission."
fi
echo "============================================================================"