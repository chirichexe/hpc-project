#!/bin/bash
#SBATCH --account=tra25_Inginfbo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=48
#SBATCH -o weak_scaling_mpi.out

EXCHANGE_MODE=$1
if [ -z "$EXCHANGE_MODE" ]; then
    EXCHANGE_MODE=0
fi

EXEC="./bin/binarize_mpi"
SEED=1234
RESULTS_DIR="./results"

mkdir -p $RESULTS_DIR

TASKS=(1 2 4 8 12 16 20 24 48)
SIZES=(5000 7071 10000 14142 17320 20000 22360 24494 34641)

echo "Run,P,N,Time" > $RESULTS_DIR/weak_mpi_results_${EXCHANGE_MODE}.csv

for i in "${!TASKS[@]}"; do
    P=${TASKS[$i]}
    N=${SIZES[$i]}
    
    for run in {1..3}; do
        RESULT=$(srun $EXEC $N $SEED $EXCHANGE_MODE -b -q)
        echo "$run,$RESULT" >> $RESULTS_DIR/weak_mpi_results_${EXCHANGE_MODE}.csv
    done
done