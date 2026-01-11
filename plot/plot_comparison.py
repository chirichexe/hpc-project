import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

def load_and_process(filename, is_weak):
    if not os.path.exists(filename):
        print(f"Errore: File {filename} non trovato")
        return None
    
    df = pd.read_csv(filename)
    # Raggruppa per numero di processi e calcola media e deviazione standard
    stats = df.groupby('P')['Time'].agg(['mean', 'std']).reset_index()
    
    # Valori base per P=1
    t1 = stats[stats['P'] == 1]['mean'].iloc[0]
    s1 = stats[stats['P'] == 1]['std'].iloc[0]
    
    if is_weak:
        # Speedup di Gustafson
        stats['speedup'] = stats['P'] * (t1 / stats['mean'])
    else:
        # Speedup di Amdahl
        stats['speedup'] = t1 / stats['mean']

    # Propagazione dell'errore per lo speedup
    stats['speedup_std'] = stats['speedup'] * np.sqrt(
        (stats['std'] / stats['mean'])**2 + (s1 / t1)**2
    )
    return stats

def plot_comparison_combined(datasets, labels, title_suffix, filename, is_weak):
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c'] # Blu, Arancio, Verde
    
    # Creazione figura 16:9 con due sottografici (1 riga, 2 colonne)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 9))
    fig.suptitle(f'Analisi {title_suffix}', fontsize=20, fontweight='bold')

    # --- 1. Sottografico del Tempo ---
    for i, stats in enumerate(datasets):
        if stats is not None:
            ax1.errorbar(stats['P'], stats['mean'], yerr=stats['std'], 
                        fmt='-o', capsize=5, label=labels[i], color=colors[i])
    
    ax1.set_xlabel('Numero di Processi (P)', fontsize=12)
    ax1.set_ylabel('Tempo (s)', fontsize=12)
    ax1.set_title('Confronto Tempi', fontsize=14)
    ax1.grid(True, which="both", ls="-", alpha=0.5)
    ax1.legend()

    # --- 2. Sottografico dello Speedup ---
    for i, stats in enumerate(datasets):
        if stats is not None:
            ax2.errorbar(stats['P'], stats['speedup'], yerr=stats['speedup_std'], 
                        fmt='-o', capsize=5, label=labels[i], color=colors[i])
    
    # Linea Ideale
    valid_data = [s for s in datasets if s is not None]
    if valid_data:
        max_p = max([s['P'].max() for s in valid_data])
        ax2.plot([1, max_p], [1, max_p], '--', color='gray', label='Ideale')
    
    ax2.set_xlabel('Numero di Processi (P)', fontsize=12)
    ax2.set_ylabel('Speedup', fontsize=12)
    ax2.set_title('Confronto Speedup', fontsize=14)
    ax2.grid(True, which="both", ls="-", alpha=0.5)
    ax2.legend()

    # Ottimizzazione spazi e salvataggio
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(filename, dpi=300)
    plt.close()

if __name__ == "__main__":
    if len(sys.argv) != 7:
        print("Uso: python plot.py s1 s2 s3 w1 w2 w3")
        print("s1, s2, s3: CSV Strong Scaling (Ssend, Isend, Sendrecv)")
        print("w1, w2, w3: CSV Weak Scaling (Ssend, Isend, Sendrecv)")
        sys.exit(1)
    
    labels = ['Ssend/Recv', 'Isend/Irecv', 'Sendrecv']
    
    # Caricamento dati
    strong_data = [load_and_process(sys.argv[i], False) for i in range(1, 4)]
    weak_data = [load_and_process(sys.argv[i], True) for i in range(4, 7)]
    
    # Generazione dei due file richiesti
    plot_comparison_combined(strong_data, labels, "Strong Scaling", "strong_comparison.png", False)
    plot_comparison_combined(weak_data, labels, "Weak Scaling", "weak_comparison.png", True)
    
    print("Operazione completata con successo:")
    print("- strong_comparison.png (16:9)")
    print("- weak_comparison.png (16:9)")