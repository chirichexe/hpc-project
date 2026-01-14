#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH -o weak_scaling_omp.out

EXEC="./bin/binarize_omp"
SEED=1234
RESULTS_DIR="./results"

mkdir -p $RESULTS_DIR

# Numero di thread e dimensioni N corrispondenti (N_base * sqrt(T))
THREADS=(1     2     4     8     12    16    20    24    48)
SIZES=(5000 7071 10000 14142 17320 20000 22360 24494 34641)

echo "Run,P,N,Time" > $RESULTS_DIR/weak_omp_results.csv

for i in "${!THREADS[@]}"; do
    T=${THREADS[$i]}
    N=${SIZES[$i]}
    export OMP_NUM_THREADS=$T
    
    for run in {1..3}; do
        RESULT=$($EXEC $T $N $SEED -b -q)
        echo "$run,$RESULT" >> $RESULTS_DIR/weak_omp_results.csv
    done
done