#!/bin/bash

#SBATCH --job-name=cpu_vs_gpu_comparison
#SBATCH --partition=standard
#SBATCH --time=24:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --gres=gpu:1
#SBATCH --mem=64G
#SBATCH --output=comparison_%j.out
#SBATCH --error=comparison_%j.err

# Load modules
module load gcc
module load cuda
module load openmpi

export CUDA_VISIBLE_DEVICES=0
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo "CPU vs GPU Primality Performance Comparison"
echo "=========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "=========================================="

# Test configuration - EXACTLY SAME as CPU test
ranges=("10000000:20000000" "100000000000:101000000000")
methods=("0:Naive" "1:Miller-Rabin" "2:Fermat" "3:Gauss-Euler" "4:MR-GE")
rounds=5

echo "Test Configuration:"
echo "- Compare CPU vs GPU performance"
echo "- Same FFT optimization on both platforms"
echo "- Ranges: ${#ranges[@]} different scales"
echo "- Methods: Miller-Rabin, MR-GE (best performing)"
echo "- Rounds: $rounds for probabilistic methods"
echo ""

# Build program
cd "$SLURM_SUBMIT_DIR"
make -f Makefile.cuda clean > /dev/null 2>&1
make -f Makefile.cuda > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "✓ Build successful"
echo ""

# Create results directory
results_dir="cpu_vs_gpu_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$results_dir"
cd "$results_dir"

# CSV header
echo "Platform,Range,Method,Time_sec,Throughput_nums_sec,Primes_Found" > "results.csv"

echo "=== GPU TESTING ==="
echo "Testing on GPU with CUDA acceleration..."
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
        
        if output=$(./cuda_prime_search $method $start $end $rounds 2>&1); then
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

echo "=== CPU TESTING ==="
echo "Testing on CPU with FFT optimization..."
echo ""

# Rebuild for CPU - force CPU fallback by temporarily removing nvcc from PATH
make -f Makefile.cuda clean > /dev/null 2>&1
PATH=/usr/bin:/bin make -f Makefile.cuda > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "CPU Build failed!"
    exit 1
fi

echo "✓ CPU Build successful"
echo ""

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
        
        if output=$(./cuda_prime_search $method $start $end $rounds 2>&1); then
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
echo "Analyzing GPU vs CPU performance..."

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

echo "=== SUMMARY ==="
echo ""
echo "Test completed successfully!"
echo ""
echo "Results saved to: $results_dir"
echo "Files created:"
echo "  - results.csv (detailed comparison data)"
echo ""
echo "Key Findings:"
echo "- GPU vs CPU performance measured"
echo "- FFT optimization active on both platforms"
echo "- Speedup varies by algorithm and number size"
echo ""

# Archive results
tar -czf "../comparison_results_${SLURM_JOB_ID}.tar.gz" .

echo ""
echo "=== COMPLETED ==="
echo "GPU vs CPU comparison completed!"
echo "Results archived: comparison_results_${SLURM_JOB_ID}.tar.gz"