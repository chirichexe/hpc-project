#!/bin/bash

# Definiamo i percorsi dei binari
SERIAL_BIN="./bin/binarize_serial"
MPI_BIN="./bin/binarize_mpi"

# File temporanei per il confronto
OUT_SERIAL=".tmp_serial.out"
OUT_MPI=".tmp_mpi.out"

echo "--- Avvio verifica correttezza ---"

# 1. Esecuzione versione Seriale
if [ -f "$SERIAL_BIN" ]; then
    echo "Esecuzione $SERIAL_BIN..."
    $SERIAL_BIN > "$OUT_SERIAL"
else
    echo "ERRORE: $SERIAL_BIN non trovato."
    exit 1
fi

# 2. Esecuzione versione MPI (usando srun come nel tuo batch)
if [ -f "$MPI_BIN" ]; then
    echo "Esecuzione $MPI_BIN con srun..."
    srun $MPI_BIN > "$OUT_MPI"
else
    echo "ERRORE: $MPI_BIN non trovato."
    exit 1
fi

# 3. Confronto degli output
echo "Confronto in corso..."

if diff "$OUT_SERIAL" "$OUT_MPI" > /dev/null; then
    echo "SUCCESS: Gli output sono identici!"
    # Pulizia file temporanei
    rm "$OUT_SERIAL" "$OUT_MPI"
    exit 0
else
    echo "****************************************"
    echo "ERRORE: Gli output sono differenti!"
    echo "****************************************"
    # Mostra le prime righe di differenza per debug
    diff "$OUT_SERIAL" "$OUT_MPI" | head -n 20
    
    # Opzionale: commenta la riga sotto se vuoi tenere i file per ispezionarli
    rm "$OUT_SERIAL" "$OUT_MPI"
    exit 1
fi