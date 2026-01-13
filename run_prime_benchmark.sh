#!/bin/bash
#SBATCH --job-name=prime_fft_benchmark
#SBATCH --partition=standard
#SBATCH --time=02:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --gres=gpu:1
#SBATCH --mem=32G
#SBATCH --output=prime_benchmark_%j.out
#SBATCH --error=prime_benchmark_%j.err

# ============================================================================
# SLURM Batch Script for Prime Number FFT Benchmarking
# Virginia Tech ARC Clusters
# ============================================================================

# ============================================================================
# CONFIGURATION FLAGS - Modify these to change benchmark parameters
# ============================================================================

# === PRIMALITY TESTING METHODS ===
# Set to 1 to enable, 0 to disable
ENABLE_NAIVE=1           # Method 0: Naive check
ENABLE_MILLER_RABIN=1     # Method 1: Miller-Rabin
ENABLE_FERMAT=1           # Method 2: Fermat  
ENABLE_GAUSS_EULER=1       # Method 3: Gauss-Euler
ENABLE_MR_GE=1            # Method 4: Miller-Rabin + Gauss-Euler

# === FFT CONFIGURATION ===
# Set to 1 to enable FFT optimization, 0 to disable
FFT_ENABLED=1              # 1 = FFT ON, 0 = FFT OFF

# === TEST RANGES ===
# Configure number ranges to test (comma-separated)
# Format: "start1:end1, start2:end2, ..."
TEST_RANGES="1000000:2000000, 10000000:11000000, 100000000:101000000, 1000000000:1000100000"

# === SAMPLING PARAMETERS ===
# Number of rounds for probabilistic methods
SAMPLING_ROUNDS=10        # Miller-Rabin and Fermat rounds

# Number of iterations for timing accuracy
TIMING_ITERATIONS=3        # Run each test this many times and average

# === OUTPUT CONFIGURATION ===
RESULTS_DIR="benchmark_results_$(date +%Y%m%d_%H%M%S)"
DETAILED_LOGGING=1         # 1 = detailed per-iteration logs, 0 = summary only

# ============================================================================
# ADVANCED CONFIGURATION - Modify only if you know what you're doing
# ============================================================================

# Build configuration
BUILD_CLEAN=1              # 1 = clean build, 0 = incremental build
MAKE_FLAGS="-j$(nproc)"   # Parallel build flags

# GPU settings
GPU_ARCH="sm_70"           # CUDA architecture (adjust for your GPU)
CUDA_VISIBLE_DEVICES=0      # Which GPU to use

# Performance monitoring
MONITOR_GPU=1              # 1 = monitor GPU usage, 0 = no monitoring
MONITOR_CPU=1              # 1 = monitor CPU usage, 0 = no monitoring

# =============================================================================
# BENCHMARK EXECUTION - Do not modify below this line
# =============================================================================

# Load required modules (VT ARC specific)
module load gcc
module load cuda
module load openmpi

# Set environment variables
export CUDA_VISIBLE_DEVICES=$GPU_VISIBLE_DEVICES
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo "=========================================================================="
echo "Prime Number FFT Benchmark - Virginia Tech ARC Cluster"
echo "=========================================================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Partition: $SLURM_JOB_PARTITION"
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "CPUs per task: $SLURM_CPUS_PER_TASK"
echo "GPU: $CUDA_VISIBLE_DEVICES"
echo "Start time: $(date)"
echo "=========================================================================="

# Print configuration
echo ""
echo "BENCHMARK CONFIGURATION:"
echo "======================="
echo "FFT Enabled: $FFT_ENABLED"
echo "Sampling Rounds: $SAMPLING_ROUNDS"
echo "Timing Iterations: $TIMING_ITERATIONS"
echo "Test Ranges: $TEST_RANGES"
echo ""
echo "Enabled Methods:"
echo "  Naive: $ENABLE_NAIVE"
echo "  Miller-Rabin: $ENABLE_MILLER_RABIN"
echo "  Fermat: $ENABLE_FERMAT"
echo "  Gauss-Euler: $ENABLE_GAUSS_EULER"
echo "  MR-GE: $ENABLE_MR_GE"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"
cd "$RESULTS_DIR"

# Function to build the program
build_program() {
    echo "Building CUDA prime search program..."
    
    # Go back to source directory
    cd "$SLURM_SUBMIT_DIR"
    
    if [ "$BUILD_CLEAN" -eq 1 ]; then
        make -f Makefile.cuda clean
    fi
    
    # Configure FFT build
    if [ "$FFT_ENABLED" -eq 1 ]; then
        export CFLAGS="-DHAVE_CUDA -DFFT_ENABLED=1"
        echo "Building with FFT optimization enabled..."
    else
        export CFLAGS="-DHAVE_CUDA -DFFT_ENABLED=0"
        echo "Building with FFT optimization disabled..."
    fi
    
    # Build
    if make -f Makefile.cuda $MAKE_FLAGS; then
        echo "Build successful!"
        return 0
    else
        echo "Build failed!"
        return 1
    fi
}

# Function to run a single benchmark test
run_benchmark() {
    local method=$1
    local method_name=$2
    local start_range=$3
    local end_range=$4
    
    echo "Running benchmark: $method_name on range [$start_range, $end_range)"
    
    # Calculate count
    local count=$((end_range - start_range))
    
    # Results file
    local results_file="${method_name}_${start_range}_${end_range}.csv"
    
    # Create CSV header
    echo "Iteration,Method,FFT,Start,End,Count,Rounds,Time_sec,Throughput_nums_sec,Primes_Found" > "$results_file"
    
    # Run multiple iterations
    for ((i=1; i<=TIMING_ITERATIONS; i++)); do
        echo "  Iteration $i/$TIMING_ITERATIONS..."
        
        # Run the benchmark
        local start_time=$(date +%s.%N)
        
        if [ "$DETAILED_LOGGING" -eq 1 ]; then
            ./cuda_prime_search "$method" "$start_range" "$end_range" "$SAMPLING_ROUNDS" > "${results_file%.csv}_iter${i}.log" 2>&1
            local exit_code=$?
        else
            ./cuda_prime_search "$method" "$start_range" "$end_range" "$SAMPLING_ROUNDS" > /dev/null 2>&1
            local exit_code=$?
        fi
        
        local end_time=$(date +%s.%N)
        local elapsed=$(echo "$end_time - $start_time" | bc -l)
        
        if [ $exit_code -eq 0 ]; then
            # Extract throughput and primes found from output
            local output=""
            if [ "$DETAILED_LOGGING" -eq 1 ]; then
                output=$(cat "${results_file%.csv}_iter${i}.log")
            fi
            
            local throughput=0
            local primes_found=0
            
            if [[ $output =~ "Throughput:"[[:space:]]*([0-9.]+) ]]; then
                throughput="${BASH_REMATCH[1]}"
            fi
            
            if [[ $output =~ "Primes found:"[[:space:]]*([0-9]+) ]]; then
                primes_found="${BASH_REMATCH[1]}"
            fi
            
            # If regex failed, calculate from time
            if [ "$throughput" = "0" ]; then
                throughput=$(echo "scale=2; $count / $elapsed" | bc -l)
            fi
            
            # Write to CSV
            echo "$i,$method_name,$FFT_ENABLED,$start_range,$end_range,$count,$SAMPLING_ROUNDS,$elapsed,$throughput,$primes_found" >> "$results_file"
            
            echo "    Time: ${elapsed}s, Throughput: ${throughput} nums/sec, Primes: $primes_found"
        else
            echo "    FAILED (exit code: $exit_code)"
            echo "$i,$method_name,$FFT_ENABLED,$start_range,$end_range,$count,$SAMPLING_ROUNDS,FAILED,0,0" >> "$results_file"
        fi
    done
    
    echo "  Benchmark completed: $results_file"
}

# Function to parse test ranges
parse_ranges() {
    local ranges_string=$1
    local -a ranges_array
    
    IFS=',' read -ra ranges_array <<< "$ranges_string"
    
    for range in "${ranges_array[@]}"; do
        # Trim whitespace
        range=$(echo "$range" | xargs)
        
        if [[ $range =~ ([0-9]+):([0-9]+) ]]; then
            local start="${BASH_REMATCH[1]}"
            local end="${BASH_REMATCH[2]}"
            run_range_tests "$start" "$end"
        else
            echo "Warning: Invalid range format: $range"
        fi
    done
}

# Function to run tests for a specific range
run_range_tests() {
    local start_range=$1
    local end_range=$2
    
    echo ""
    echo "Testing range: [$start_range, $end_range)"
    echo "=================================="
    
    # Test each enabled method
    if [ "$ENABLE_NAIVE" -eq 1 ]; then
        run_benchmark 0 "Naive" "$start_range" "$end_range"
    fi
    
    if [ "$ENABLE_MILLER_RABIN" -eq 1 ]; then
        run_benchmark 1 "Miller-Rabin" "$start_range" "$end_range"
    fi
    
    if [ "$ENABLE_FERMAT" -eq 1 ]; then
        run_benchmark 2 "Fermat" "$start_range" "$end_range"
    fi
    
    if [ "$ENABLE_GAUSS_EULER" -eq 1 ]; then
        run_benchmark 3 "Gauss-Euler" "$start_range" "$end_range"
    fi
    
    if [ "$ENABLE_MR_GE" -eq 1 ]; then
        run_benchmark 4 "MR-GE" "$start_range" "$end_range"
    fi
}

# Function to generate summary report
generate_summary() {
    echo ""
    echo "Generating summary report..."
    
    local summary_file="benchmark_summary.csv"
    
    # Create summary header
    echo "FFT_Enabled,Method,Range_Start,Range_End,Avg_Time_sec,Avg_Throughput,Std_Dev_Throughput,Min_Throughput,Max_Throughput" > "$summary_file"
    
    # Process each results file
    for csv_file in *.csv; do
        if [[ "$csv_file" != "benchmark_summary.csv" ]]; then
            # Extract method and range from filename
            local base_name=$(basename "$csv_file" .csv)
            if [[ $base_name =~ ^(.*)_([0-9]*)_([0-9]*)$ ]]; then
                local method="${BASH_REMATCH[1]}"
                local start="${BASH_REMATCH[2]}"
                local end="${BASH_REMATCH[3]}"
                
                # Calculate statistics
                local avg_time=$(awk -F',' 'NR>1 && $8!="FAILED" {sum+=$8; count++} END {if(count>0) print sum/count; else print 0}' "$csv_file")
                local avg_throughput=$(awk -F',' 'NR>1 && $9!="0" {sum+=$9; count++} END {if(count>0) print sum/count; else print 0}' "$csv_file")
                local min_throughput=$(awk -F',' 'NR>1 && $9!="0" {if(min==0 || $9<min) min=$9} END {print min}' "$csv_file")
                local max_throughput=$(awk -F',' 'NR>1 && $9!="0" {if($9>max) max=$9} END {print max}' "$csv_file")
                local std_dev=$(awk -F',' 'NR>1 && $9!="0" {sum+=$9; sumsq+=$9*$9; count++} END {if(count>0) {mean=sum/count; var=sumsq/count-mean*mean; print sqrt(var)} else print 0}' "$csv_file")
                
                echo "$FFT_ENABLED,$method,$start,$end,$avg_time,$avg_throughput,$std_dev,$min_throughput,$max_throughput" >> "$summary_file"
            fi
        fi
    done
    
    echo "Summary saved to: $summary_file"
}

# Function to monitor system resources
monitor_resources() {
    if [ "$MONITOR_GPU" -eq 1 ] || [ "$MONITOR_CPU" -eq 1 ]; then
        echo "Starting resource monitoring..."
        
        local monitor_file="resource_monitor.log"
        echo "Timestamp,CPU_Usage,Mem_Usage,GPU_Usage,GPU_Memory,Temperature" > "$monitor_file"
        
        # Monitor in background
        while true; do
            local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
            local cpu_usage=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | sed 's/%us,//')
            local mem_usage=$(free | grep Mem | awk '{printf "%.1f", $3/$2 * 100.0}')
            
            if [ "$MONITOR_GPU" -eq 1 ]; then
                local gpu_info=$(nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits | head -1)
                local gpu_util=$(echo "$gpu_info" | cut -d',' -f1)
                local gpu_mem_used=$(echo "$gpu_info" | cut -d',' -f2)
                local gpu_mem_total=$(echo "$gpu_info" | cut -d',' -f3)
                local gpu_temp=$(echo "$gpu_info" | cut -d',' -f4)
                
                echo "$timestamp,$cpu_usage,$mem_usage,$gpu_util,$gpu_mem_used/$gpu_mem_total,$gpu_temp" >> "$monitor_file"
            else
                echo "$timestamp,$cpu_usage,$mem_usage,NA,NA,NA" >> "$monitor_file"
            fi
            
            sleep 5
        done &
        
        MONITOR_PID=$!
        echo "Resource monitoring PID: $MONITOR_PID"
    fi
}

# Main execution
main() {
    # Build the program
    if ! build_program; then
        echo "Failed to build program. Exiting."
        exit 1
    fi
    
    # Return to results directory
    cd "$RESULTS_DIR"
    
    # Start resource monitoring
    monitor_resources
    
    # Save configuration
    {
        echo "Benchmark Configuration"
        echo "====================="
        echo "Job ID: $SLURM_JOB_ID"
        echo "Start Time: $(date)"
        echo "FFT Enabled: $FFT_ENABLED"
        echo "Sampling Rounds: $SAMPLING_ROUNDS"
        echo "Timing Iterations: $TIMING_ITERATIONS"
        echo "Test Ranges: $TEST_RANGES"
        echo ""
        echo "Enabled Methods:"
        echo "  Naive: $ENABLE_NAIVE"
        echo "  Miller-Rabin: $ENABLE_MILLER_RABIN"
        echo "  Fermat: $ENABLE_FERMAT"
        echo "  Gauss-Euler: $ENABLE_GAUSS_EULER"
        echo "  MR-GE: $ENABLE_MR_GE"
        echo ""
        echo "System Information:"
        echo "  Node: $HOSTNAME"
        echo "  CPU Cores: $SLURM_CPUS_PER_TASK"
        echo "  GPU: $CUDA_VISIBLE_DEVICES"
        echo "  Memory: $SLURM_MEM_PER_NODE"
        echo ""
        echo "Module Versions:"
        module list 2>&1
        echo ""
        echo "GCC Version:"
        gcc --version | head -1
        echo ""
        if command -v nvcc &> /dev/null; then
            echo "NVCC Version:"
            nvcc --version | grep release
        fi
    } > configuration.txt
    
    # Run benchmarks
    parse_ranges "$TEST_RANGES"
    
    # Stop monitoring
    if [ ! -z "$MONITOR_PID" ]; then
        kill $MONITOR_PID 2>/dev/null
        echo "Stopped resource monitoring."
    fi
    
    # Generate summary
    generate_summary
    
    echo ""
    echo "=========================================================================="
    echo "BENCHMARK COMPLETED"
    echo "=========================================================================="
    echo "Results saved to: $RESULTS_DIR"
    echo "End time: $(date)"
    
    # Create results archive
    tar -czf "../prime_benchmark_${SLURM_JOB_ID}.tar.gz" .
    echo "Results archived: prime_benchmark_${SLURM_JOB_ID}.tar.gz"
    
    echo ""
    echo "Files generated:"
    ls -la
}

# Check for required tools
if ! command -v bc &> /dev/null; then
    echo "Error: 'bc' calculator is required but not installed."
    echo "Please load the appropriate module or install bc."
    exit 1
fi

# Run main function
main