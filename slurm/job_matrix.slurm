#!/bin/bash
#SBATCH --job-name=matrix_transform      # Job name
#SBATCH --output=matrix_transform_%j.out # Standard output file (%j expands to job ID)
#SBATCH --error=matrix_transform_%j.err  # Standard error file
#SBATCH --nodes=2                        # Number of nodes
#SBATCH --ntasks-per-node=4              # Number of MPI tasks per node
#SBATCH --cpus-per-task=1                # Number of CPU cores per task
#SBATCH --time=00:30:00                  # Maximum execution time (HH:MM:SS)
#SBATCH --mem=8GB                        # Memory per node
#SBATCH --partition=normal               # Partition/queue name (adjust as needed)

# Load required modules (adjust based on your cluster environment)
# module load openmpi
# module load gcc

# Print job information
echo "=========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Job Name: $SLURM_JOB_NAME"
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "Tasks: $SLURM_NTASKS"
echo "CPUs per task: $SLURM_CPUS_PER_TASK"
echo "Working directory: $(pwd)"
echo "=========================================="
echo ""

# Display the start time
echo "Job started at: $(date)"
echo ""

# Matrix size (default N=2000, can be modified)
MATRIX_SIZE=2000

# Run the MPI program
echo "Running matrix transformation with N=$MATRIX_SIZE..."
mpirun -np $SLURM_NTASKS ./matrix_transform $MATRIX_SIZE

# Check exit status
if [ $? -eq 0 ]; then
    echo ""
    echo "Job completed successfully at: $(date)"
else
    echo ""
    echo "Job failed at: $(date)"
    exit 1
fi

echo "=========================================="
