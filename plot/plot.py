import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def plot_scaling(filename, output_name):
    if not os.path.exists(filename):
        print(f"Errore: {filename} non trovato")
        return
    
    # Caricamento dati
    df = pd.read_csv(filename)
    
    # Setup figura (16:9)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 8))
    
    # --- Plot 1: Speedup ---
    # Rimosso errorbar e colonna 'std', uso un plot semplice
    ax1.plot(df['P'], df['speedup'], '-o', color='tab:blue', label='Speedup Sperimentale')
    
    # Linea di Speedup Ideale
    ax1.plot(df['P'], df['P']/df['P'].min(), '--', color='gray', label='Ideale')

    ax1.set_xlabel('Numero di Nodi (P)')
    ax1.set_ylabel('Speedup')
    ax1.set_title(f'Analisi dello Speedup')
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.legend()

    # --- Plot 2: Efficienza ---
    ax2.plot(df['P'], df['efficiency'], '-s', color='tab:red', label='Efficienza')
    ax2.axhline(y=1.0, color='gray', linestyle='--', label='Ideale (1.0)')
    
    ax2.set_xlabel('Numero di Nodi (P)')
    ax2.set_ylabel('Efficienza')
    ax2.set_title(f'Analisi dell\'Efficienza')
    ax2.set_ylim(0, 1.1)
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.legend()

    plt.tight_layout()
    plt.savefig(output_name, dpi=300)
    print(f"Grafico salvato: {output_name}")
    plt.close()

if __name__ == "__main__":
    # Controllo argomenti: script + strong + weak + type = 4
    if len(sys.argv) != 4:
        print("Utilizzo: python plot.py <strong.csv> <weak.csv> <type>")
        sys.exit(1)
    
    strong_csv = sys.argv[1]
    weak_csv = sys.argv[2]
    plot_type = sys.argv[3] 
    
    # Generazione nomi file: <type>_strong.png e <type>_weak.png
    strong_output = f"{plot_type}_strong.png"
    weak_output = f"{plot_type}_weak.png"
    
    plot_scaling(strong_csv, strong_output)
    plot_scaling(weak_csv, weak_output)