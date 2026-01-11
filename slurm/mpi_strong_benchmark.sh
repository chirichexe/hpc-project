#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=24
#SBATCH -o strong_scaling.out

EXCHANGE_MODE=$1

if [ -z "$EXCHANGE_MODE" ]; then
    EXCHANGE_MODE=0
fi

EXEC="./bin/binarize_mpi"
SEED=1234
N_FIXED=10000
RESULTS_DIR="./results"

mkdir -p $RESULTS_DIR

echo "Run,P,N,Time" > $RESULTS_DIR/strong_mpi_results_${EXCHANGE_MODE}.csv

for P in 1 2 4 8 12 16 20 24 48; do
    for i in {1..3}; do
        RESULT=$(srun -n $P $EXEC $N_FIXED $SEED $EXCHANGE_MODE -b --q)
        echo "$i,$RESULT" >> $RESULTS_DIR/strong_mpi_results_${EXCHANGE_MODE}.csv
    done
done