#!/bin/bash
#SBATCH --account=tra24_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:05:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1 # Run a single task per node
#SBATCH -c 24 # number of CPU cores i.e. OpenMP threads per task
#SBATCH -o job.out
#SBATCH -e job.err
for I in 12 24 48; do
echo "Launching binary $I"
srun ./bin/calculateR $I
done