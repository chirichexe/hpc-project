#!/bin/bash

SERIAL_BIN="./bin/binarize_serial"
MPI_BIN="./bin/binarize_mpi"
SIZE=2000
SEED=42 # Seed identico per entrambi

OUT_SERIAL=".tmp_serial.out"
OUT_MPI=".tmp_mpi.out"

echo "--- Verifica correttezza (Size: $SIZE, Seed: $SEED) ---"

# Esecuzione Seriale (passiamo size e seed)
# Filtriamo l'output per tenere solo le righe con 0 e 1
$SERIAL_BIN $SIZE $SEED | grep -E '^[01 ]+$' > "$OUT_SERIAL"

# Esecuzione MPI (stessi parametri)
srun --ntasks=24 $MPI_BIN $SIZE $SEED | grep -E '^[01 ]+$' > "$OUT_MPI"

echo "Confronto in corso..."

if diff -q "$OUT_SERIAL" "$OUT_MPI" > /dev/null; then
    echo "SUCCESS: Gli output coincidono perfettamente!"
    rm "$OUT_SERIAL" "$OUT_MPI"
    exit 0
else
    echo "ERRORE: Differenza riscontrata nei dati."
    # Mostra la riga esatta dove iniziano i problemi
    diff -y --suppress-common-lines "$OUT_SERIAL" "$OUT_MPI" | head -n 10
    exit 1
fi