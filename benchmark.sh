#!/bin/bash
#SBATCH --account=tra25_IngInfBo
#SBATCH --partition=g100_usr_prod
#SBATCH -t 00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks=24
#SBATCH --ntasks-per-node=24
#SBATCH --cpus-per-task=1
#SBATCH -o job.out
#SBATCH -e job.err

# Carica eventuali moduli necessari (es. intel o openmpi)
# module load openmpi

# --- TEST MPI ---
for I in 1080 2160 4320; do
    srun ./bin/binarize_MPI $I
done
