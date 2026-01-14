#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH --time=00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=48
#SBATCH --ntasks-per-node=48
#SBATCH --output=strong_scaling_mpi.out

EXCHANGE_MODE=$1
if [ -z "$EXCHANGE_MODE" ]; then
    EXCHANGE_MODE=0
fi

EXEC="./bin/binarize_mpi"
SEED=1234
N=10000
RESULTS_DIR="./results"

mkdir -p "$RESULTS_DIR"

echo "Run,P,N,Time" > "$RESULTS_DIR/strong_mpi_results_${EXCHANGE_MODE}.csv"

# Strong scaling MPI intra-nodo
for P in 1 2 4 8 12 16 20 24 48; do
    for i in {1..3}; do
        RESULT=$(srun --ntasks=$P $EXEC $N $SEED $EXCHANGE_MODE -b -q)
        echo "$i,$RESULT" >> "$RESULTS_DIR/strong_mpi_results_${EXCHANGE_MODE}.csv"
    done
done
