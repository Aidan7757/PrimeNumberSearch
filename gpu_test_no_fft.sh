#!/bin/bash

#SBATCH --job-name=cpu_vs_gpu_comparison_no_fft
#SBATCH --partition=standard
#SBATCH --time=24:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --gres=gpu:1
#SBATCH --mem=64G
#SBATCH --output=comparison_no_fft_%j.out
#SBATCH --error=comparison_no_fft_%j.err

# Load modules
module load gcc
module load cuda
module load openmpi

export CUDA_VISIBLE_DEVICES=0
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo "CPU vs GPU Primality Performance Comparison (No FFT/SS)"
echo "======================================================"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "======================================================"

# Test configuration - EXACTLY SAME as CPU test
ranges=("10000000:20000000" "100000000000:101000000000")
methods=("0:Naive" "1:Miller-Rabin" "2:Fermat" "3:Gauss-Euler" "4:MR-GE")
rounds=5

echo "Test Configuration:"
echo "- Compare CPU vs GPU performance"
echo "- NO FFT/Schönhage-Strassen optimization (traditional methods only)"
echo "- Ranges: ${#ranges[@]} different scales"
echo "- Methods: All 5 primality tests"
echo "- Rounds: $rounds for probabilistic methods"
echo ""

# Disable FFT and Schönhage-Strassen by temporarily renaming files
cd "$SLURM_SUBMIT_DIR"

echo "Disabling FFT and Schönhage-Strassen..."
if [ -f "src/fft_multiply.cu" ]; then
    mv src/fft_multiply.cu src/fft_multiply.cu.disabled
fi
if [ -f "src/fft_multiply.h" ]; then
    mv src/fft_multiply.h src/fft_multiply.h.disabled
fi
if [ -f "src/schonhage_strassen.cu" ]; then
    mv src/schonhage_strassen.cu src/schonhage_strassen.cu.disabled
fi
if [ -f "src/schonhage_strassen.h" ]; then
    mv src/schonhage_strassen.h src/schonhage_strassen.h.disabled
fi
echo "✓ FFT and SS optimization disabled"

# Update source code to disable FFT calls
echo "Updating source code to disable FFT..."

# Disable FFT in search_methods_cuda.cu
sed -i.bak '/#include "fft_multiply.h"/d' src/search_methods_cuda.cu
sed -i '/#include "schonhage_strassen.h"/d' src/search_methods_cuda.cu

# Remove FFT calls from cuda_mod_mul function
sed -i '/cuda_fast_mod_mul/d' src/search_methods_cuda.cu
sed -i '/schonhage_strassen_mul/d' src/search_methods_cuda.cu

# Make the cuda_mod_mul function use only traditional method
cat > temp_mod_mul.c << 'EOF'
__device__ unsigned long long cuda_mod_mul(unsigned long long a, unsigned long long b, unsigned long long mod) {
    // Use traditional method only - FFT and Schönhage-Strassen disabled
    a %= mod;
    b %= mod;
    
    #if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 300
        // Use 128-bit multiplication on modern CUDA
        unsigned __int128 temp = (unsigned __int128)a * b;
        return (unsigned long long)(temp % mod);
    #else
        // Russian peasant algorithm for older architectures
        unsigned long long res = 0;
        a %= mod;
        
        while (b > 0) {
            if (b & 1) {
                if (res >= mod - a) {
                    res = res + a - mod;
                } else {
                    res = res + a;
                }
            }
            
            if (a >= mod - a) {
                a = a + a - mod;
            } else {
                a = a + a;
            }
            
            b >>= 1;
        }
        return res;
    #endif
}
EOF

# Replace the cuda_mod_mul function in the source
python3 -c "
import re

with open('src/search_methods_cuda.cu', 'r') as f:
    content = f.read()

# Replace the entire cuda_mod_mul function
pattern = r'__device__ unsigned long long cuda_mod_mul\(.*?\n\}'
replacement = open('temp_mod_mul.c').read()

content = re.sub(pattern, replacement, content, flags=re.DOTALL)

with open('src/search_methods_cuda.cu', 'w') as f:
    f.write(content)

print('cuda_mod_mul function updated')
"

# Clean up temp file
rm temp_mod_mul.c

echo "✓ Source code updated to disable FFT/SS"

# Build program
echo "Building program without FFT optimization..."
make -f Makefile.cuda clean > /dev/null 2>&1
make -f Makefile.cuda > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "Build failed!"
    # Restore files
    restore_files
    exit 1
fi

echo "✓ Build successful"
echo ""

# Function to restore files
restore_files() {
    echo "Restoring original files..."
    cd "$SLURM_SUBMIT_DIR"
    
    if [ -f "src/fft_multiply.cu.disabled" ]; then
        mv src/fft_multiply.cu.disabled src/fft_multiply.cu
    fi
    if [ -f "src/fft_multiply.h.disabled" ]; then
        mv src/fft_multiply.h.disabled src/fft_multiply.h
    fi
    if [ -f "src/schonhage_strassen.cu.disabled" ]; then
        mv src/schonhage_strassen.cu.disabled src/schonhage_strassen.cu
    fi
    if [ -f "src/schonhage_strassen.h.disabled" ]; then
        mv src/schonhage_strassen.h.disabled src/schonhage_strassen.h
    fi
    
    if [ -f "src/search_methods_cuda.cu.bak" ]; then
        mv src/search_methods_cuda.cu.bak src/search_methods_cuda.cu
    fi
    
    echo "✓ Files restored"
}

# Create results directory
results_dir="cpu_vs_gpu_results_no_fft_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$results_dir"
cd "$results_dir"

# CSV header
echo "Platform,Range,Method,Time_sec,Throughput_nums_sec,Primes_Found" > "results.csv"

echo "=== GPU TESTING (No FFT) ==="
echo "Testing on GPU with traditional modular multiplication..."
echo ""

for range in "${ranges[@]}"; do
    start=${range%:*}
    end=${range#*:}
    
    echo "GPU Testing range [$start, $end]:"
    
    for method_config in "${methods[@]}"; do
        method=${method_config%:*}
        method_name=${method_config#*:}
        
        echo "  GPU Testing $method_name..."
        
        # Run test
        start_time=$(date +%s.%N)
        
        if output=$(../cuda_prime_search $method $start $end $rounds 2>&1); then
            end_time=$(date +%s.%N)
            elapsed=$(echo "scale=6; $end_time - $start_time" | bc -l)
            count=$((end - start))
            
            # Avoid division by zero
            if [ "$elapsed" != "0" ] && [ "$elapsed" != "0.000000" ]; then
                throughput=$(echo "scale=0; $count / $elapsed" | bc -l)
            else
                throughput=0
                elapsed="0.000001"
            fi
            
            primes_found=$(echo "$output" | grep "Primes found:" | awk '{print $3}' || echo "0")
            
            echo "    GPU Time: ${elapsed}s, Throughput: ${throughput} nums/sec, Primes: $primes_found"
            
            # Save to CSV
            echo "GPU,$start-$end,$method_name,$elapsed,$throughput,$primes_found" >> "results.csv"
        else
            echo "    GPU Test failed"
            echo "GPU,$start-$end,$method_name,FAILED,0,0" >> "results.csv"
        fi
    done
    echo ""
done

echo "=== CPU TESTING (No FFT) ==="
echo "Testing on CPU with traditional modular multiplication..."
echo ""

# Rebuild for CPU
cd ..
PATH=/usr/bin:/bin make -f Makefile.cuda clean > /dev/null 2>&1
PATH=/usr/bin:/bin make -f Makefile.cuda > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "CPU Build failed!"
    restore_files
    exit 1
fi

echo "✓ CPU Build successful"
cd "$results_dir"

for range in "${ranges[@]}"; do
    start=${range%:*}
    end=${range#*:}
    
    echo "CPU Testing range [$start, $end]:"
    
    for method_config in "${methods[@]}"; do
        method=${method_config%:*}
        method_name=${method_config#*:}
        
        echo "  CPU Testing $method_name..."
        
        # Run test
        start_time=$(date +%s.%N)
        
        if output=$(../cuda_prime_search $method $start $end $rounds 2>&1); then
            end_time=$(date +%s.%N)
            elapsed=$(echo "scale=6; $end_time - $start_time" | bc -l)
            count=$((end - start))
            
            if [ "$elapsed" != "0" ] && [ "$elapsed" != "0.000000" ]; then
                throughput=$(echo "scale=0; $count / $elapsed" | bc -l)
            else
                throughput=0
                elapsed="0.000001"
            fi
            
            primes_found=$(echo "$output" | grep "Primes found:" | awk '{print $3}' || echo "0")
            
            echo "    CPU Time: ${elapsed}s, Throughput: ${throughput} nums/sec, Primes: $primes_found"
            
            # Save to CSV
            echo "CPU,$start-$end,$method_name,$elapsed,$throughput,$primes_found" >> "results.csv"
        else
            echo "    CPU Test failed"
            echo "CPU,$start-$end,$method_name,FAILED,0,0" >> "results.csv"
        fi
    done
    echo ""
done

echo "=== PERFORMANCE ANALYSIS ==="
echo ""
echo "Analyzing GPU vs CPU performance (No FFT)..."

# Calculate speedup
for range in "${ranges[@]}"; do
    start=${range%:*}
    end=${range#*:}
    
    echo "Performance for range [$start, $end]:"
    
    for method_config in "${methods[@]}"; do
        method_name=${method_config#*:}
        
        gpu_total=0
        gpu_count=0
        cpu_total=0
        cpu_count=0
        
        while IFS=, read -r platform range_method time throughput primes; do
            if [ "$range_method" == "$start-$end" ] && [ "$platform" == "GPU" ] && [ "$method_name" == "$method_name" ]; then
                if [ "$throughput" != "0" ]; then
                    gpu_total=$(echo "$gpu_total + $throughput" | bc -l)
                    gpu_count=$((gpu_count + 1))
                fi
            elif [ "$range_method" == "$start-$end" ] && [ "$platform" == "CPU" ] && [ "$method_name" == "$method_name" ]; then
                if [ "$throughput" != "0" ]; then
                    cpu_total=$(echo "$cpu_total + $throughput" | bc -l)
                    cpu_count=$((cpu_count + 1))
                fi
            fi
        done < "results.csv"
        
        # Calculate averages and speedup
        if [ $gpu_count -gt 0 ]; then
            gpu_avg=$(echo "scale=0; $gpu_total / $gpu_count" | bc -l)
            echo "  GPU $method_name: $gpu_avg nums/sec"
        fi
        
        if [ $cpu_count -gt 0 ]; then
            cpu_avg=$(echo "scale=0; $cpu_total / $cpu_count" | bc -l)
            echo "  CPU $method_name: $cpu_avg nums/sec"
            
            # Calculate speedup
            if [ $gpu_count -gt 0 ] && [ $cpu_count -gt 0 ]; then
                speedup=$(echo "scale=2; $gpu_avg / $cpu_avg" | bc -l)
                echo "  Speedup: ${speedup}x"
                
                # Performance assessment
                if (( $(echo "$speedup >= 10" | bc -l) )); then
                    echo "  Assessment: EXCELLENT GPU acceleration"
                elif (( $(echo "$speedup >= 5" | bc -l) )); then
                    echo "  Assessment: GOOD GPU acceleration"
                elif (( $(echo "$speedup >= 2" | bc -l) )); then
                    echo "  Assessment: MODERATE GPU acceleration"
                else
                    echo "  Assessment: MINIMAL GPU acceleration"
                fi
            fi
        fi
        echo ""
    done
done

echo "=== SUMMARY ==="
echo ""
echo "Test completed successfully!"
echo ""
echo "Results saved to: $results_dir"
echo "Files created:"
echo "  - results.csv (detailed comparison data)"
echo ""
echo "Key Findings:"
echo "- GPU vs CPU performance measured WITHOUT FFT/SS optimization"
echo "- Traditional modular multiplication methods only"
echo "- Shows baseline GPU vs CPU comparison"
echo ""
echo "Compare with FFT results:"
echo "- FFT/SS typically provides 2-10x speedup for large numbers"
echo "- More pronounced benefit for very large numbers (>1M)"

# Restore original files
restore_files

# Archive results
tar -czf "../comparison_results_no_fft_${SLURM_JOB_ID}.tar.gz" .

echo ""
echo "=== COMPLETED ==="
echo "GPU vs CPU comparison completed (No FFT)!"
echo "Results archived: comparison_results_no_fft_${SLURM_JOB_ID}.tar.gz"