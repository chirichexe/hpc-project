#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

#define N 2000 

int main(int argc, char* argv[]) {
    int n_size = N;
    unsigned int seed = (unsigned int)time(NULL);
    int quiet = 0;

    // Semplice parsing: primo arg = size, secondo = seed
    if (argc > 1) n_size = atoi(argv[1]);
    if (argc > 2) seed = atoi(argv[2]);

    MPI_Init(&argc, &argv);
    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (n_size % size != 0) {
        if (my_rank == 0) printf("Error: n_size %d not divisible by %d\n", n_size, size);
        MPI_Finalize(); return EXIT_FAILURE;
    }

    int rows_per_proc = n_size / size;
    int *A_raw = NULL;
    int *T_raw = NULL;

    if (my_rank == 0) {
        A_raw = malloc(n_size * n_size * sizeof(int));
        T_raw = malloc(n_size * n_size * sizeof(int));
        srand(seed);
        for (int i = 0; i < n_size * n_size; i++) A_raw[i] = rand() % 10;
    }

    int *my_A = malloc(rows_per_proc * n_size * sizeof(int));
    int *my_T = malloc(rows_per_proc * n_size * sizeof(int));

    MPI_Scatter(A_raw, rows_per_proc * n_size, MPI_INT, my_A, rows_per_proc * n_size, MPI_INT, 0, MPI_COMM_WORLD);

    // GHOST CELLS ALLOCATION
    int *my_A_ghosts = calloc((rows_per_proc + 2) * n_size, sizeof(int));
    memcpy(my_A_ghosts + n_size, my_A, rows_per_proc * n_size * sizeof(int));

    int up = (my_rank == 0) ? MPI_PROC_NULL : my_rank - 1;
    int down = (my_rank == size - 1) ? MPI_PROC_NULL : my_rank + 1;

    // Scambio righe: invio la mia prima riga a "up", ricevo in "bottom ghost" da "down"
    MPI_Sendrecv(my_A_ghosts + n_size, n_size, MPI_INT, up, 0, 
                 my_A_ghosts + (rows_per_proc + 1) * n_size, n_size, MPI_INT, down, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    // Invio la mia ultima riga a "down", ricevo in "top ghost" da "up"
    MPI_Sendrecv(my_A_ghosts + rows_per_proc * n_size, n_size, MPI_INT, down, 1, 
                 my_A_ghosts, n_size, MPI_INT, up, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (int i = 0; i < rows_per_proc; i++) {
        for (int j = 0; j < n_size; j++) {
            float sum = 0; int count = 0;
            int global_i = my_rank * rows_per_proc + i;

            for (int z = i - 1; z <= i + 1; z++) {
                for (int w = j - 1; w <= j + 1; w++) {
                    int target_z = z + 1; // Spostamento per via della riga ghost in cima
                    if (w >= 0 && w < n_size) {
                        // Controllo bordi globali (top/bottom della matrice intera)
                        if ((global_i == 0 && z < i) || (global_i == n_size - 1 && z > i)) continue;
                        
                        sum += my_A_ghosts[target_z * n_size + w];
                        count++;
                    }
                }
            }
            my_T[i * n_size + j] = (my_A[i * n_size + j] * count > sum) ? 1 : 0;
        }
    }

    MPI_Gather(my_T, rows_per_proc * n_size, MPI_INT, T_raw, rows_per_proc * n_size, MPI_INT, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        for (int i = 0; i < n_size; i++) {
            for (int j = 0; j < n_size; j++) printf("%d ", T_raw[i * n_size + j]);
            printf("\n");
        }
        free(A_raw); free(T_raw);
    }

    free(my_A); free(my_T); free(my_A_ghosts);
    MPI_Finalize();
    return 0;
}