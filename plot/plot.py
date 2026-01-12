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
        
        # Riferimento (P=1)
        t1 = stats[stats['P'] == 1]['mean'].iloc[0]
        s1 = stats[stats['P'] == 1]['std'].iloc[0]
        
        # Applicazione formule specifiche richieste
        if is_weak:
            # Speedup di Gustafson: P * (T1 / Tp)
            stats['speedup'] = stats['P'] * (t1 / stats['mean'])
            color = "green"
        else:
            # Speedup di Amdahl: T1 / Tp
            stats['speedup'] = t1 / stats['mean']
            color = "orange"

        # Propagazione dell'errore per lo speedup
        # La formula della propagazione rimane valida per entrambi i casi 
        # (moltiplicare per P sposta solo la scala, non l'errore relativo)
        stats['speedup_std'] = stats['speedup'] * np.sqrt(
            (stats['std'] / stats['mean'])**2 + (s1 / t1)**2
        )

        # Creazione della figura: 1 riga, 2 colonne
        fig, (ax_time, ax_speedup) = plt.subplots(1, 2, figsize=(15, 6))

        # --- SINISTRA: Grafico Tempi ---
        ax_time.errorbar(stats['P'], stats['mean'], yerr=stats['std'], 
                         fmt='-o', capsize=5, label='Tempo medio')
        ax_time.set_xlabel('Numero di Processi (P)')
        ax_time.set_ylabel('Tempo (s)')
        ax_time.set_title('Confronto Tempi')
        ax_time.grid(True, which="both", ls="-", alpha=0.5)
        ax_time.legend()

        # --- DESTRA: Grafico Speedup ---
        ax_speedup.errorbar(stats['P'], stats['speedup'], yerr=stats['speedup_std'], 
                            fmt='-o', capsize=5, color=color, label='Osservato')
        
        # In entrambi i casi (Gustafson e Amdahl), lo speedup ideale è lineare (y = P)
        ax_speedup.plot(stats['P'], stats['P'], '--', color='gray', label='Ideale (Lineare)')
            
        ax_speedup.set_xlabel('Numero di Processi (P)')
        ax_speedup.set_ylabel('Speedup')
        ax_speedup.set_title('Confronto Speedup')
        ax_speedup.grid(True, which="both", ls="-", alpha=0.5)
        ax_speedup.legend()

        # Ottimizzazione spazi e salvataggio
        plt.tight_layout()
        output_file = f"{base_name}_combined.png"
        plt.savefig(output_file)
        plt.close()
        print(f"Generato: {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python plot.py strong_results.csv weak_results.csv")
        sys.exit(1)
    
    process_scaling(sys.argv[1], sys.argv[2])