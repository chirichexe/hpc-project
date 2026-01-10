#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=24
#SBATCH -o strong_scaling.out

EXEC="../bin/binarize_mpi"
SEED=1234
N_FIXED=2000
RESULTS_DIR="results"

mkdir -p $RESULTS_DIR

echo "Run,P,N,Time" > $RESULTS_DIR/strong_mpi_results.csv

for P in 1 2 4 8 12 16 20 24; do
    for i in {1..10}; do
        RESULT=$(srun -n $P $EXEC $N_FIXED $SEED -b --q)
        echo "$i,$RESULT" >> $RESULTS_DIR/strong_mpi_results.csv
    done
done