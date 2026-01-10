import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

def process_scaling(strong_csv, weak_csv):
    files = [(strong_csv, False), (weak_csv, True)]
    
    for filename, is_weak in files:
        if not os.path.exists(filename):
            print(f"Errore: File {filename} non trovato")
            continue
            
        df = pd.read_csv(filename)
        base_name = os.path.splitext(filename)[0]
        stats = df.groupby('P')['Time'].agg(['mean', 'std']).reset_index()
        
        t1 = stats[stats['P'] == 1]['mean'].iloc[0]
        s1 = stats[stats['P'] == 1]['std'].iloc[0]
        
        if is_weak:
            stats['speedup'] = stats['P'] * (t1 / stats['mean'])
            title_suffix = "Weak Scaling (Gustafson)"
            color = "green"
        else:
            stats['speedup'] = t1 / stats['mean']
            title_suffix = "Strong Scaling"
            color = "orange"

        stats['speedup_std'] = stats['speedup'] * np.sqrt(
            (stats['std'] / stats['mean'])**2 + (s1 / t1)**2
        )

        # Plot Tempo
        plt.figure(figsize=(10, 6))
        plt.errorbar(stats['P'], stats['mean'], yerr=stats['std'], fmt='-o', capsize=5, label='Tempo medio')
        plt.xlabel('Numero di Processi (P)')
        plt.ylabel('Tempo (s)')
        plt.title(f'Scaling del Tempo: {title_suffix}')
        plt.grid(True, which="both", ls="-", alpha=0.5)
        plt.legend()
        plt.savefig(f"{base_name}_time.png")
        plt.close()

        # Plot Speedup
        plt.figure(figsize=(10, 6))
        plt.errorbar(stats['P'], stats['speedup'], yerr=stats['speedup_std'], fmt='-o', capsize=5, color=color, label='Osservato')
        plt.plot(stats['P'], stats['P'], '--', color='gray', label='Ideale')
        plt.xlabel('Numero di Processi (P)')
        plt.ylabel('Speedup')
        plt.title(f'Scaling dello Speedup: {title_suffix}')
        plt.grid(True, which="both", ls="-", alpha=0.5)
        plt.legend()
        plt.savefig(f"{base_name}_speedup.png")
        plt.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python script.py strong_results.csv weak_results.csv")
        sys.exit(1)
    
    process_scaling(sys.argv[1], sys.argv[2])
    print("Grafici generati con successo.")