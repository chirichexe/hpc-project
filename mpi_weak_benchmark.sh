#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=24
#SBATCH -o weak_scaling.out

EXEC="./bin/binarize_mpi"
SEED=1234

echo "P,N,Time" > weak_mpi_results.csv

# P: numero di task
# N: calcolato come N_base * sqrt(P) per mantenere N^2/P costante
TASKS=(1     2     4     8     12    16    24)
SIZES=(10000 14142 20000 28284 34641 40000 48989)

for i in "${!TASKS[@]}"; do
    P=${TASKS[$i]}
    N=${SIZES[$i]}
    srun -n $P $EXEC $N $SEED -b --q >> weak_mpi_results.csv
done