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
        # Speedup di Gustafson: P * (T1 / Tp) -> semplificato spesso come T1/Tp se il carico per proc è costante
        # Usiamo la formula coerente col tuo script originale
        stats['speedup'] = stats['P'] * (t1 / stats['mean'])
    else:
        # Speedup di Amdahl: T1 / Tp
        stats['speedup'] = t1 / stats['mean']

    # Propagazione dell'errore per lo speedup
    stats['speedup_std'] = stats['speedup'] * np.sqrt(
        (stats['std'] / stats['mean'])**2 + (s1 / t1)**2
    )
    return stats

def plot_comparison(datasets, labels, title_suffix, filename_prefix, is_weak):
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c'] # Blu, Arancio, Verde
    
    # 1. Plot del Tempo
    plt.figure(figsize=(10, 6))
    for i, stats in enumerate(datasets):
        if stats is not None:
            plt.errorbar(stats['P'], stats['mean'], yerr=stats['std'], 
                         fmt='-o', capsize=5, label=labels[i], color=colors[i])
    
    plt.xlabel('Numero di Processi (P)')
    plt.ylabel('Tempo (s)')
    plt.title(f'Confronto Tempi: {title_suffix}')
    plt.grid(True, which="both", ls="-", alpha=0.5)
    plt.legend()
    plt.savefig(f"{filename_prefix}_time_comparison.png")
    plt.close()

    # 2. Plot dello Speedup
    plt.figure(figsize=(10, 6))
    for i, stats in enumerate(datasets):
        if stats is not None:
            plt.errorbar(stats['P'], stats['speedup'], yerr=stats['speedup_std'], 
                         fmt='-o', capsize=5, label=labels[i], color=colors[i])
    
    # Linea Ideale
    max_p = max([s['P'].max() for s in datasets if s is not None])
    plt.plot([1, max_p], [1, max_p], '--', color='gray', label='Ideale')
    
    plt.xlabel('Numero di Processi (P)')
    plt.ylabel('Speedup')
    plt.title(f'Confronto Speedup: {title_suffix}')
    plt.grid(True, which="both", ls="-", alpha=0.5)
    plt.legend()
    plt.savefig(f"{filename_prefix}_speedup_comparison.png")
    plt.close()

if __name__ == "__main__":
    if len(sys.argv) != 7:
        print("Uso: python plot.py s1 s2 s3 w1 w2 w3")
        print("s1, s2, s3: CSV Strong Scaling (Ssend, Isend, Sendrecv)")
        print("w1, w2, w3: CSV Weak Scaling (Ssend, Isend, Sendrecv)")
        sys.exit(1)
    
    labels = ['Ssend/Recv', 'Isend/Irecv', 'Sendrecv']
    
    # Processa i 3 file Strong
    strong_data = [load_and_process(sys.argv[i], False) for i in range(1, 4)]
    # Processa i 3 file Weak
    weak_data = [load_and_process(sys.argv[i], True) for i in range(4, 7)]
    
    # Genera i grafici comparativi
    plot_comparison(strong_data, labels, "Strong Scaling", "strong", False)
    plot_comparison(weak_data, labels, "Weak Scaling", "weak", True)
    
    print("Grafici comparativi generati: strong_*.png e weak_*.png")
