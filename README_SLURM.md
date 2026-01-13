# SLURM Benchmark Scripts for Prime Number FFT Testing

This directory contains SLURM batch scripts specifically configured for Virginia Tech's ARC clusters to benchmark prime number testing with and without FFT optimization.

## Quick Start

### For Immediate Testing:
1. **Edit 4 lines** in `run_prime_quick.sh`:
   ```bash
   FFT_ENABLED=1        # 1 = FFT ON, 0 = FFT OFF
   TEST_METHOD=1         # 0=Naive, 1=Miller-Rabin, 2=Fermat, 3=Gauss-Euler, 4=MR-GE
   TEST_START=1000000    # Start range
   TEST_END=2000000      # End range
   ```

2. **Submit to ARC cluster**:
   ```bash
   sbatch run_prime_quick.sh
   ```

### For Comprehensive Benchmarking:
1. **Edit configuration** in `run_prime_benchmark.sh`
2. **Submit**:
   ```bash
   sbatch run_prime_benchmark.sh
   ```

## Files Overview

| File | Purpose | Configuration |
|------|---------|---------------|
| `run_prime_quick.sh` | Quick single test | 4 easy configuration lines |
| `run_prime_benchmark.sh` | Full benchmark suite | Advanced configuration |
| `test_slurm_setup.sh` | Validate setup locally | Run before submission |
| `SLURM_CONFIG_GUIDE.md` | Detailed configuration guide | Comprehensive options |

## Configuration Options

### Quick Script (run_prime_quick.sh)
```bash
FFT_ENABLED=1                    # FFT optimization (0/1)
TEST_METHOD=1                    # Primality method (0-4)
TEST_START=1000000              # Range start
TEST_END=2000000                # Range end  
SAMPLING_ROUNDS=10              # Probabilistic rounds
```

### Comprehensive Script (run_prime_benchmark.sh)
```bash
# Method Selection
ENABLE_NAIVE=1                  # Enable/disable specific methods
ENABLE_MILLER_RABIN=1
ENABLE_FERMAT=1
ENABLE_GAUSS_EULER=1
ENABLE_MR_GE=1

# FFT Configuration
FFT_ENABLED=1                   # Global FFT toggle

# Test Ranges (comma-separated)
TEST_RANGES="1000000:2000000, 10000000:11000000, 100000000:101000000"

# Sampling Parameters
SAMPLING_ROUNDS=10             # Probabilistic test rounds
TIMING_ITERATIONS=3            # Average over multiple runs
```

## Primality Testing Methods

| Method ID | Name | Type | Best For |
|-----------|------|------|----------|
| 0 | Naive | Deterministic | Small numbers, verification |
| 1 | Miller-Rabin | Probabilistic | General purpose, fast |
| 2 | Fermat | Probabilistic | Quick testing, educational |
| 3 | Gauss-Euler | Deterministic | Mathematical rigor |
| 4 | MR-GE | Hybrid | Best of both worlds |

## Performance Testing Scenarios

### 1. FFT vs Non-FFT Comparison:
```bash
# Test 1: FFT ON
FFT_ENABLED=1
TEST_METHOD=1
TEST_START=1000000
TEST_END=2000000

# Test 2: FFT OFF  
FFT_ENABLED=0
TEST_METHOD=1
TEST_START=1000000
TEST_END=2000000
```

### 2. Method Performance Comparison:
```bash
ENABLE_NAIVE=0           # Skip slow method
ENABLE_MILLER_RABIN=1
ENABLE_FERMAT=1
ENABLE_GAUSS_EULER=1
ENABLE_MR_GE=1
```

### 3. Scale Testing:
```bash
TEST_RANGES="1000:2000, 1000000:2000000, 1000000000:1000100000"
```

## ARC Cluster Specifics

### Resource Requests:
- **Standard Job**: `#SBATCH --partition=standard --time=01:00:00 --mem=16G`
- **GPU Job**: `#SBATCH --gres=gpu:1 --cpus-per-task=8`
- **High Memory**: `#SBATCH --partition=highmem --mem=128G`

### Module Loading:
```bash
module load gcc cuda openmpi
```

### Job Monitoring:
```bash
squeue -u $USER           # Your jobs
tail -f prime_quick_*.out  # Real-time output
```

## Expected Results

### Output Files:
- **Quick Test**: `prime_quick_12345.out`, `prime_quick_12345.err`
- **Full Benchmark**: `benchmark_results_12345/` directory containing:
  - `benchmark_summary.csv` - Aggregated performance data
  - `configuration.txt` - Job settings and system info
  - `resource_monitor.log` - CPU/GPU usage during testing
  - `Method_Range.csv` files - Detailed per-iteration data

### Performance Metrics:
- **Time per number** (seconds)
- **Throughput** (numbers/second)  
- **Primes found** (count)
- **Resource usage** (CPU/GPU)

## Usage Examples

### Example 1: Quick FFT Test
```bash
# Edit run_prime_quick.sh:
FFT_ENABLED=1
TEST_METHOD=1
TEST_START=10000000
TEST_END=10100000

# Submit:
sbatch run_prime_quick.sh
```

### Example 2: Compare All Methods
```bash
# Edit run_prime_benchmark.sh:
ENABLE_NAIVE=0          # Skip naive (too slow)
FFT_ENABLED=1
TEST_RANGES="1000000:2000000, 10000000:11000000"

# Submit:
sbatch run_prime_benchmark.sh
```

### Example 3: Large Scale Test
```bash
# Edit script resources:
#SBATCH --partition=highmem
#SBATCH --mem=64G
#SBATCH --time=04:00:00

TEST_RANGES="1000000000:1001000000"
SAMPLING_ROUNDS=20
```

## Results Analysis

### Performance Comparisons:
The scripts generate CSV files perfect for analysis:

```csv
FFT_Enabled,Method,Range_Start,Range_End,Avg_Throughput,Std_Dev_Throughput
1,Miller-Rabin,1000000,2000000,2500000,125000
0,Miller-Rabin,1000000,2000000,2000000,100000
```

### Expected Findings:
- **FFT provides 20-30% improvement** for large numbers
- **Miller-Rabin** is typically fastest for general use
- **Naive method** becomes impractical beyond small numbers
- **GPU parallelization** provides order-of-magnitude speedups

## Troubleshooting

### Common Issues:
1. **Build Failures**: Ensure modules loaded: `module load gcc cuda`
2. **Out of Memory**: Reduce range size or increase `--mem`
3. **GPU Not Available**: Check partition and `--gres=gpu` allocation
4. **Job Timeouts**: Increase `--time` limit

### Quick Test Before Submission:
```bash
./test_slurm_setup.sh
```

## Advanced Usage

### Array Jobs for Multiple Tests:
```bash
#SBATCH --array=0-9
# Each array task tests different parameters
```

### Custom GPU Types:
```bash
#SBATCH --constraint=v100     # V100 GPUs only
#SBATCH --constraint=a100     # A100 GPUs only  
```

### Email Notifications:
```bash
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=your_email@vt.edu
```

## Support

### Virginia Tech ARC Help:
- **Documentation**: https://www.arc.vt.edu/
- **Support**: arc-support@vt.edu
- **Status**: https://status.arc.vt.edu/

### Module Help:
```bash
module spider cuda          # Find available CUDA versions
module help gcc           # Get help with GCC module
```

---

**Ready to run!** The scripts are validated and configured for Virginia Tech's ARC clusters. Upload the files, edit your configuration, and submit with `sbatch`.