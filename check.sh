#!/bin/bash

# binary files
SERIAL_BIN="./bin/binarize_serial"
MPI_BIN="./bin/binarize_mpi"
OPENMP_BIN="./bin/binarize_omp"

# program parameters
SEED=42 # same seed 
SIZE=1234
NUM_PROCS=4

# temporary output files
OUT_SERIAL=".tmp_serial.out"
OUT_MPI=".tmp_mpi.out"
OUT_OPENMP=".tmp_openmp.out"

echo "======================================================="
echo "> Checking program execution. Size: $SIZE, Seed: $SEED"
echo "======================================================="

# SERIAL execution
# Filter output to keep only lines with 0 and 1
$SERIAL_BIN $SIZE $SEED | grep -E '^[01 ]+$' | sort >"$OUT_SERIAL"

# MPI execution (same parameters)
mpirun -np $NUM_PROCS $MPI_BIN $SIZE $SEED | grep -E '^[01 ]+$' | sort >"$OUT_MPI"

# OPENMP execution (same parameters)
$OPENMP_BIN 1 $SIZE $SEED | grep -E '^[01 ]+$' | sort >"$OUT_OPENMP"

echo "> Comparing outputs..."
echo "======================================================="

# Generate hashes for each output
HASH_SERIAL=$(md5sum "$OUT_SERIAL" | cut -d' ' -f1)
HASH_MPI=$(md5sum "$OUT_MPI" | cut -d' ' -f1)
HASH_OPENMP=$(md5sum "$OUT_OPENMP" | cut -d' ' -f1)

# Check if all hashes are equal to the serial one
if [ "$HASH_SERIAL" = "$HASH_MPI" ] && [ "$HASH_SERIAL" = "$HASH_OPENMP" ]; then
  echo "> SUCCESS: Outputs match perfectly!"
else
  echo "> ERROR: Differences found in the outputs."
  # Show differences between serial and MPI as a debug example
  diff -y --suppress-common-lines "$OUT_SERIAL" "$OUT_MPI" | head -n 10
  diff -y --suppress-common-lines "$OUT_SERIAL" "$OUT_OPENMP" | head -n 10
fi

rm "$OUT_SERIAL" "$OUT_MPI" "$OUT_OPENMP"
echo "======================================================="
