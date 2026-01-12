import pandas as pd
import sys

if len(sys.argv) < 4:
    print("Usage: python calculate_scaling.py <file.csv> <weak|strong> <mpi|openmp>")
    sys.exit(1)

file_in, mode, parallel_model = sys.argv[1], sys.argv[2], sys.argv[3]
df = pd.read_csv(file_in)

# group by P and N, calculate mean and std
stats = df.groupby(['P', 'N'])['Time'].agg(['mean', 'std']).reset_index()

# get base time for P=1
t1 = stats.loc[stats['P'] == stats['P'].min(), 'mean'].values[0]

# scaling analysis
if mode == 'weak':
    stats['speedup'] = stats['P'] * (t1 / stats['mean'])
else:
    stats['speedup'] = t1 / stats['mean']

# calculate efficiency
stats['efficiency'] = stats['speedup'] / stats['P']

# determine output filename
out_map = {
    ('weak', 'mpi'): 'weak_mpi.csv',
    ('weak', 'openmp'): 'weak_openmp.csv',
    ('strong', 'mpi'): 'strong_mpi.csv',
    ('strong', 'openmp'): 'strong_openmp.csv',
}
out_file = out_map.get((mode, parallel_model))
if out_file is None:
    print("Invalid mode or parallel model. Use weak|strong and mpi|openmp.")
    sys.exit(1)

stats.to_csv(out_file, index=False)
print(f"Analysis {mode} completed. File created: {out_file}")