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

TASKS=(1 2 4 8 16 24)
#SIZES=(2000 2828 4000 5656 8000 9798 10984)
SIZES=(10000 20000 40000 80000 120000 160000 240000) 

for i in "${!TASKS[@]}"; do
    P=${TASKS[$i]}
    N=${SIZES[$i]}
    srun -n $P $EXEC $N $SEED -b --q >> weak_mpi_results.csv
done