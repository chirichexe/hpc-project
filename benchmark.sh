#!/bin/bash
#SBATCH --account=tra24_IngInfBo
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
echo "--- INIZIO TEST MPI ---"
for I in 1000 2000 4000; do
    srun -n 24 ./bin/bynarize_mpi $I
done
