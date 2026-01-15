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
    
    # Valori base per P=min(P) (solitamente 1)
    p_min = stats['P'].min()
    t1 = stats[stats['P'] == p_min]['mean'].iloc[0]
    s1 = stats[stats['P'] == p_min]['std'].iloc[0]
    
    if is_weak:
        # Speedup di Gustafson: P * (T1 / Tp)
        # Nota: T1 è il tempo su 1 processo con carico base, 
        # Tp è il tempo su P processi con carico P volte maggiore.
        stats['speedup'] = (t1 / stats['mean']) * (stats['P'] / p_min)
    else:
        # Speedup di Amdahl: T1 / Tp
        stats['speedup'] = t1 / stats['mean']

    # Propagazione dell'errore per lo speedup
    stats['speedup_std'] = stats['speedup'] * np.sqrt(
        (stats['std'] / stats['mean'])**2 + (s1 / t1)**2
    ).fillna(0) # evita NaN se lo std è zero
    
    return stats

def plot_single_scaling(stats, title, filename, is_weak):
    if stats is None:
        return

    # Creazione figura 16:9 con due sottografici
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 9))
    #fig.suptitle(title, fontsize=18, fontweight='bold')

    # --- 1. Sottografico del Tempo ---
    ax1.errorbar(stats['P'], stats['mean'], yerr=stats['std'], 
                fmt='-o', capsize=5, color='#1f77b4', label='Tempo misurato')
    
    ax1.set_xlabel('Numero di Nodi (P)', fontsize=12)
    ax1.set_ylabel('Tempo (s)', fontsize=12)
    ax1.set_title('Andamento Tempi', fontsize=14)
    ax1.grid(True, which="both", ls="-", alpha=0.5)
    ax1.legend()

    # --- 2. Sottografico dello Speedup ---
    ax2.errorbar(stats['P'], stats['speedup'], yerr=stats['speedup_std'], 
                fmt='-o', capsize=5, color='#ff7f0e', label='Speedup sperimentale')
    
    # Linea Ideale
    max_p = stats['P'].max()
    p_min = stats['P'].min()
    ax2.plot([p_min, max_p], [1, max_p/p_min], '--', color='gray', label='Ideale')
    
    ax2.set_xlabel('Numero di Nodi (P)', fontsize=12)
    ax2.set_ylabel('Speedup', fontsize=12)
    ax2.set_title('Andamento Speedup', fontsize=14)
    ax2.grid(True, which="both", ls="-", alpha=0.5)
    ax2.legend()

    # Ottimizzazione spazi e salvataggio
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(filename, dpi=300)
    print(f"Grafico salvato: {filename}")
    plt.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python plot.py <file_strong.csv> <file_weak.csv>")
        sys.exit(1)
    
    strong_file = sys.argv[1]
    weak_file = sys.argv[2]
    
    # Caricamento e processing
    data_strong = load_and_process(strong_file, False)
    data_weak = load_and_process(weak_file, True)
    
    # Generazione dei grafici
    if data_strong is not None:
        plot_single_scaling(data_strong, "Strong Scaling Analysis", "strong_scaling.png", False)
    
    if data_weak is not None:
        plot_single_scaling(data_weak, "Weak Scaling Analysis", "weak_scaling.png", True)
