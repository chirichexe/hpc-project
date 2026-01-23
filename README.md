# HPC Project: Parallel Binarization

Parallel implementation (MPI & OpenMP) of a local-mean thresholding algorithm for large matrices ($N \ge 2000$).

## Assignment Overview
* **Goal:** Convert matrix $A$ to binary matrix $T$ based on $3 \times 3$ local average.

* **Logic:** $t_{ij} = 1$ if $a_{ij} > m_{ij}$, else $t_{ij} = 0$.
* **MPI:** Distributed memory with row-wise decomposition and halo exchange for boundaries.
* **OpenMP:** Shared memory parallelization.

* **Analysis:** Performance evaluation (Strong and Weak scaling) on CINECA Galileo100.

---

## Quick Start

### 1. Build
```bash
module load intel openmpi
make
```

### 2. Execute Benchmarks

Submit jobs to the Slurm scheduler:
```bash
sbatch slurm/mpi_strong_benchmark.sh
sbatch slurm/mpi_weak_benchmark.sh
sbatch slurm/openmp_strong_benchmark.sh
sbatch slurm/openmp_weak_benchmark.sh
```

### 3. Visualization

**Single Mode Plot:**
```bash
python plot.py results/strong_mpi_results_0.csv results/weak_mpi_results_0.csv
```

**Comparison Plot (Ssend vs Isend vs Sendrecv):**
```bash
# Usage: python plot_comparison.py s1 s2 s3 w1 w2 w3
python plot_comparison.py \
    results/strong_mpi_results_0.csv results/strong_mpi_results_1.csv results/strong_mpi_results_2.csv \
    results/weak_mpi_results_0.csv results/weak_mpi_results_1.csv results/weak_mpi_results_2.csv
```

**Calculate Speedup and Efficiency:**
```bash
python plot/calculate.py results/strong_mpi_results_0.csv
```

---

## Technical Specifications

### Local Mean Calculation
V
For each element $a_{ij}$, the mean $m_{ij}$ is:
$$m_{ij} = \frac{1}{9}\sum_{k=i-1}^{i+1}\sum_{h=j-1}^{j+1}a_{kh}$$

### Requirements

* **MPI Solution:** Each node calculates a portion of T. Matrix A is distributed. Master node aggregates results. Boundary elements must be handled.

* **OpenMP Solution:** Matrices A and T are shared. Workload is split among threads.

* **Scalability Analysis:**
  * Strong Scaling: Fixed N, increasing P.
  * Weak Scaling: Increasing N proportional to P.

### File Structure

* **bin/:** Compiled executables.
* **src/:** Source code.
* **slurm/:** Slurm batch scripts.
* **results/:** CSV benchmark outputs.
* **docs/:** Typst documentation.
* **check.sh - plot/:** Verification, calculating, and plotting utilities.
* **data/:** Output data from calculations.