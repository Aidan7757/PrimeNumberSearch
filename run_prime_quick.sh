#!/bin/bash
#SBATCH --job-name=prime_fft_quick
#SBATCH --partition=standard
#SBATCH --time=01:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --gres=gpu:1
#SBATCH --mem=16G
#SBATCH --output=prime_quick_%j.out
#SBATCH --error=prime_quick_%j.err

# ============================================================================
# QUICK CONFIGURATION SLURM Script - Virginia Tech ARC Clusters
# Simple version for quick testing with minimal configuration
# ============================================================================

# === QUICK CONFIGURATION - Modify these 4 lines only ===
FFT_ENABLED=1                    # 1 = FFT ON, 0 = FFT OFF
TEST_METHOD=1                    # 0=Naive, 1=Miller-Rabin, 2=Fermat, 3=Gauss-Euler, 4=MR-GE
TEST_START=1000000              # Start of number range
TEST_END=2000000                # End of number range
SAMPLING_ROUNDS=10              # Rounds for probabilistic methods

# === OPTIONAL: Test multiple ranges (comma-separated) ===
# TEST_RANGES="1000000:1100000, 10000000:10100000, 100000000:101000000"

# ============================================================================
# EXECUTION - Do not modify below
# ============================================================================

module load gcc
module load cuda
module load openmpi

export CUDA_VISIBLE_DEVICES=0
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

echo "Prime Number FFT Benchmark - Quick Test"
echo "===================================="
echo "Job ID: $SLURM_JOB_ID"
echo "FFT Enabled: $FFT_ENABLED"
echo "Method: $TEST_METHOD"
echo "Range: [$TEST_START, $TEST_END)"
echo "Rounds: $SAMPLING_ROUNDS"
echo "Start time: $(date)"
echo ""

# Build and run
cd "$SLURM_SUBMIT_DIR"

if [ "$FFT_ENABLED" -eq 1 ]; then
    echo "Building with FFT optimization..."
    make -f Makefile.cuda clean
    make -f Makefile.cuda
else
    echo "Building without FFT optimization..."
    make -f Makefile.cuda clean
    make -f Makefile.cuda CFLAGS="-DFFT_ENABLED=0"
fi

echo ""
echo "Running benchmark..."
./cuda_prime_search "$TEST_METHOD" "$TEST_START" "$TEST_END" "$SAMPLING_ROUNDS"

echo ""
echo "Benchmark completed at: $(date)"