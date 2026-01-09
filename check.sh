#!/bin/bash

SERIAL_BIN="./bin/binarize_serial"
MPI_BIN="./bin/binarize_mpi"
SIZE=2062
SEED=42 # same seed
NUM_PROCS=4

OUT_SERIAL=".tmp_serial.out"
OUT_MPI=".tmp_mpi.out"

echo "Checking for Size: $SIZE, Seed: $SEED"

# Serial execution (passing size and seed)
# Filter output to keep only lines with 0 and 1
$SERIAL_BIN $SIZE $SEED | grep -E '^[01 ]+$' >"$OUT_SERIAL"

# MPI execution (same parameters)
mpirun -np $NUM_PROCS $MPI_BIN $SIZE $SEED | grep -E '^[01 ]+$' >"$OUT_MPI"

echo "Comparing outputs..."

if diff -q "$OUT_SERIAL" "$OUT_MPI" >/dev/null; then
  echo "SUCCESS: Outputs match perfectly!"
  rm "$OUT_SERIAL" "$OUT_MPI"
  exit 0
else
  echo "ERROR: Differences found in the outputs."
  # Show the exact line where problems start
  diff -y --suppress-common-lines "$OUT_SERIAL" "$OUT_MPI" | head -n 10
  exit 1
fi
