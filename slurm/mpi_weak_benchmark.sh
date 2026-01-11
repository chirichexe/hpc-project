#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=24
#SBATCH -o weak_scaling.out

EXCHANGE_MODE=$1
if [ -z "$EXCHANGE_MODE" ]; then
    EXCHANGE_MODE=0
fi

EXEC="../bin/binarize_mpi"
SEED=1234
RESULTS_DIR="../results"

mkdir -p $RESULTS_DIR

TASKS=(1     2     4     8     12    16    20   24)
SIZES=(5000 7071 10000 14142 17320 20000 22360 24494)

echo "Run,P,N,Time" > $RESULTS_DIR/weak_mpi_results_${EXCHANGE_MODE}.csv

for i in "${!TASKS[@]}"; do
    P=${TASKS[$i]}
    N=${SIZES[$i]}
    for run in {1..3}; do
        RESULT=$(srun -n $P $EXEC $N $SEED $EXCHANGE_MODE -b --q)
        echo "$run,$RESULT" >> $RESULTS_DIR/weak_mpi_results_${EXCHANGE_MODE}.csv
    done
done