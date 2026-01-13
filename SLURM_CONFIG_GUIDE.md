# ============================================================================
# PRIME BENCHMARK CONFIGURATION GUIDE
# Virginia Tech ARC Clusters - SLURM Scripts
# ============================================================================

## QUICK START

### For Quick Testing (4 lines to configure):
Edit `run_prime_quick.sh` and modify these lines:
```bash
FFT_ENABLED=1        # Set to 0 to disable FFT
TEST_METHOD=1         # 0=Naive, 1=Miller-Rabin, 2=Fermat, 3=Gauss-Euler, 4=MR-GE
TEST_START=1000000    # Start range
TEST_END=2000000      # End range
SAMPLING_ROUNDS=10    # Probabilistic rounds
```

### Submit Job:
```bash
sbatch run_prime_quick.sh
```

## COMPREHENSIVE BENCHMARKING

### For Full Testing (edit `run_prime_benchmark.sh`):

#### 1. Configure Methods:
```bash
ENABLE_NAIVE=1           # Method 0: Naive check
ENABLE_MILLER_RABIN=1     # Method 1: Miller-Rabin  
ENABLE_FERMAT=1           # Method 2: Fermat
ENABLE_GAUSS_EULER=1       # Method 3: Gauss-Euler
ENABLE_MR_GE=1            # Method 4: Miller-Rabin + Gauss-Euler
```

#### 2. Configure FFT:
```bash
FFT_ENABLED=1              # 1 = FFT ON, 0 = FFT OFF
```

#### 3. Configure Test Ranges:
```bash
# Multiple ranges separated by commas
TEST_RANGES="1000000:2000000, 10000000:11000000, 100000000:101000000"
```

#### 4. Configure Sampling:
```bash
SAMPLING_ROUNDS=10        # Miller-Rabin and Fermat rounds
TIMING_ITERATIONS=3       # Average over multiple runs
```

### Submit Comprehensive Job:
```bash
sbatch run_prime_benchmark.sh
```

## VIRGINIA TECH ARC SPECIFICS

### Available Partitions:
- `standard` - Default partition, good for most jobs
- `gpu` - GPU-optimized partition  
- `highmem` - High memory nodes
- `short` - Short jobs (< 4 hours)

### GPU Options:
```bash
#SBATCH --gres=gpu:1        # Request 1 GPU
#SBATCH --gres=gpu:2        # Request 2 GPUs
#SBATCH --gres=gpu:v100:1    # Specific GPU type
```

### Time Limits:
```bash
#SBATCH --time=01:00:00     # 1 hour
#SBATCH --time=04:00:00     # 4 hours  
#SBATCH --time=24:00:00     # 24 hours
```

### Memory Options:
```bash
#SBATCH --mem=16G           # 16 GB memory
#SBATCH --mem=64G           # 64 GB memory
#SBATCH --mem-per-cpu=4G    # 4 GB per CPU core
```

## EXAMPLE CONFIGURATIONS

### 1. Quick FFT Performance Test:
```bash
# File: run_prime_quick.sh
FFT_ENABLED=1
TEST_METHOD=1              # Miller-Rabin
TEST_START=10000000
TEST_END=10100000  
SAMPLING_ROUNDS=15
```

### 2. FFT vs Non-FFT Comparison:
```bash
# First run (FFT ON):
FFT_ENABLED=1
TEST_METHOD=1
TEST_START=1000000
TEST_END=2000000

# Second run (FFT OFF):  
FFT_ENABLED=0
TEST_METHOD=1
TEST_START=1000000
TEST_END=2000000
```

### 3. Large Scale Benchmark:
```bash
# File: run_prime_benchmark.sh
ENABLE_NAIVE=0           # Skip slow naive method
ENABLE_MILLER_RABIN=1
ENABLE_FERMAT=1
ENABLE_GAUSS_EULER=1
ENABLE_MR_GE=1

FFT_ENABLED=1
TEST_RANGES="1000000:2000000, 10000000:11000000, 100000000:101000000, 1000000000:1000100000"
SAMPLING_ROUNDS=20
TIMING_ITERATIONS=5
```

### 4. Memory-Intensive Testing:
```bash
#SBATCH --partition=highmem
#SBATCH --mem=128G
#SBATCH --cpus-per-task=32
TEST_RANGES="1000000000:1001000000, 10000000000:10001000000"
```

## MONITORING RESULTS

### Check Job Status:
```bash
squeue -u $USER           # Your jobs
squeue -p standard         # Jobs in standard partition
```

### View Results:
```bash
# Real-time output:
tail -f prime_quick_12345.out

# Check completed jobs:
ls benchmark_results_*/
cd benchmark_results_*/ && ls -la
```

### Result Files Generated:
- `benchmark_summary.csv` - Aggregated results
- `Method_Range.csv` - Detailed per-iteration data
- `resource_monitor.log` - System resource usage
- `configuration.txt` - Job configuration details

## PERFORMANCE TIPS

### 1. Optimize for GPU:
```bash
#SBATCH --gres=gpu:v100:1    # Request V100 for best performance
#SBATCH --cpus-per-task=16    # More CPU cores per GPU
```

### 2. Batch Multiple Jobs:
```bash
# Submit different ranges in parallel:
for range in "1000000:2000000" "10000000:11000000" "100000000:101000000"; do
    # Create temporary config
    sed "s/TEST_RANGES=.*/TEST_RANGES=\"$range\"/" run_prime_benchmark.sh > temp_${range//[:]/_}.sh
    sbatch temp_${range//[:]/_}.sh
done
```

### 3. Request Specific GPU Types:
```bash
#SBATCH --constraint=v100       # Only V100 GPUs
#SBATCH --constraint=a100       # Only A100 GPUs
```

## TROUBLESHOOTING

### Common Issues:
1. **Out of Memory**: Reduce range or increase `--mem`
2. **GPU Not Available**: Check partition and `--gres=gpu`  
3. **Build Fails**: Load modules: `module load gcc cuda`
4. **Permission Denied**: `chmod +x *.sh`

### Check Available Resources:
```bash
sinfo                      # Cluster info
squeue --start              # Job start times
scontrol show partition     # Partition details
```

### Cancel Jobs:
```bash
scancel 12345              # Cancel specific job
scancel -u $USER          # Cancel all your jobs
```

## ADVANCED USAGE

### Custom Modules:
```bash
module load gcc/9.3.0
module load cuda/11.4
module load openmpi/4.1.1
```

### Array Jobs (multiple parameter sets):
```bash
#SBATCH --array=0-4
# In script:
case $SLURM_ARRAY_TASK_ID in
    0) FFT_ENABLED=0; TEST_METHOD=1 ;;   # No FFT, Miller-Rabin
    1) FFT_ENABLED=1; TEST_METHOD=1 ;;   # FFT, Miller-Rabin  
    2) FFT_ENABLED=0; TEST_METHOD=2 ;;   # No FFT, Fermat
    3) FFT_ENABLED=1; TEST_METHOD=2 ;;   # FFT, Fermat
    4) FFT_ENABLED=1; TEST_METHOD=4 ;;   # FFT, MR-GE
esac
```

### Email Notifications:
```bash
#SBATCH --mail-type=ALL
#SBATCH --mail-user=your_email@vt.edu
```