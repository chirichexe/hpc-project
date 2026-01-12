#!/bin/bash
#SBATCH --account=tra25_Inginfbo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=48
#SBATCH --exclusive
#SBATCH -o weak_scaling.out

EXCHANGE_MODE=$1
if [ -z "$EXCHANGE_MODE" ]; then
    EXCHANGE_MODE=0
fi

EXEC="./bin/binarize_mpi"
SEED=1234
RESULTS_DIR="./results"

mkdir -p $RESULTS_DIR

TASKS=(1 2 4 8 16 32 48 64 96)
SIZES=(10000 14142 20000 28284 40000 56568 69282 80000 97979)

echo "Run,P,N,Time" > $RESULTS_DIR/weak_mpi_results_${EXCHANGE_MODE}.csv

for i in "${!TASKS[@]}"; do
    P=${TASKS[$i]}
    N=${SIZES[$i]}
    
    # Determina quanti nodi servono per il numero di task corrente P
    # Poiché ntasks-per-node=48, se P > 48 servono 2 nodi, altrimenti 1.
    if [ $P -gt 48 ]; then
        CURR_NODES=2
    else
        CURR_NODES=1
    fi

    for run in {1..3}; do
        # Aggiunto --nodes=$CURR_NODES per eliminare il warning sui nodi
        RESULT=$(srun --nodes=$CURR_NODES -n $P --exact $EXEC $N $SEED $EXCHANGE_MODE -b -q)
        echo "$run,$RESULT" >> $RESULTS_DIR/weak_mpi_results_${EXCHANGE_MODE}.csv
    done
done