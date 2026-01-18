#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH -o strong_scaling_omp.out

EXEC="srun ./bin/binarize_omp"
SEED=1234
N=10000
RESULTS_DIR="./results"

mkdir -p $RESULTS_DIR

echo "Run,P,N,Time" >$RESULTS_DIR/strong_omp_results.csv

# Ciclo sul numero di thread
for T in 1 2 4 8 12 16 20 24 48; do
  export OMP_NUM_THREADS=$T
  for i in {1..3}; do
    # Esecuzione: passiamo T come primo argomento dell'eseguibile
    RESULT=$($EXEC $T $N $SEED -b -q)
    echo "$i,$RESULT" >>$RESULTS_DIR/strong_omp_results.csv
  done
done

