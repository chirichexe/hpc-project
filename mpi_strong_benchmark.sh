#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:20:00
#SBATCH --nodes=1
#SBATCH --ntasks=24
#SBATCH -o strong_scaling.out

EXEC="./bin/binarize_mpi"
SEED=1234
N_FIXED=2000 # strong scaling = same problem size

echo "P,N,Time" > strong_mpi_results.csv

# P = threads
for P in 1 2 4 8 12 16 20 24; do
    # -b (benchmark)
    # --q (quiet)
    srun -n $P $EXEC $N_FIXED $SEED -b --q >> strong_mpi_results.csv
done