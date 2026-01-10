#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

#define N 2000 

int main(int argc, char* argv[]) {

    /* SERIAL */
    // arguments parsing
    int n_size = N;
    int quiet = 0;
    int benchmark = 0;
    unsigned int seed = (unsigned int)time(NULL);
 
    int pos = 0; // 0 = size, 1 = seed
    for (int k = 1; k < argc; k++) {

        // if quiet flag, no output
        if (strcmp(argv[k], "--q") == 0) {
            quiet = 1;
            continue;
        }
        if (strcmp(argv[k], "-b") == 0 || strcmp(argv[k], "--benchmark") == 0) {
            benchmark = 1;
            continue;
        }
        char *end = NULL;
        long val = strtol(argv[k], &end, 10);
        if (*end != '\0') 
            continue;
        if (pos == 0) { // first: MATRIX SIZE
            if (val > 0) 
                n_size = (int)val;  
        }
        else if (pos == 1) { // second: SEED
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* PARALLEL */
    MPI_Init(&argc, &argv);
    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Number of rows per process (with ceiling)
    int rows_per_proc = (n_size + size - 1) / size;
    int padded_rows_total = rows_per_proc * size; // total rows with padding

    int *A_raw = NULL; // full matrix A (only on master, with padding)
    int *T_raw = NULL; // full matrix T (only on master, with padding)

    /* Data structures for Scatterv */
    // it says me how many elements each process will receive
    int *sendcounts = malloc(size * sizeof(int));
    
    // it says me the displacement (in elements) from the beginning of A_raw for each process
    int *senddispls = malloc(size * sizeof(int));

    if (my_rank == 0) { /* MASTER */
        
        // allocates and initializes the full (padded) matrix A and T
        A_raw = malloc(padded_rows_total * n_size * sizeof(int));
        T_raw = malloc(padded_rows_total * n_size * sizeof(int));
        
        // seed of the random number generator
        srand(seed);

        // matrix A generation 
        // (from 0 to n_size: real rows
        // then padding with 0
        for (int i = 0; i < padded_rows_total; i++) {
            for (int j = 0; j < n_size; j++) {
                if (i < n_size) {
                    A_raw[i * n_size + j] = rand() % 10;
                } else {
                    A_raw[i * n_size + j] = 0;
                }
            }
        }

        /* Calculation of sendcounts and senddispls */
        senddispls[0] = 0; // displacement of the first block is 0

        for (int p = 0; p < size; p++) {

            // calculate the starting row index for process p
            int global_row_start = p * rows_per_proc;
            int remaining_rows = n_size - global_row_start; // there may not be  
                                                            // enough "real" rows left
            
            int actual_rows;
            if (remaining_rows <= 0) { // no more real rows
                actual_rows = 0;

            } else if (remaining_rows < rows_per_proc) { // last process with partial rows
                actual_rows = remaining_rows;
            
            } else { // send the full block
                actual_rows = rows_per_proc;
            }
            
            sendcounts[p] = actual_rows * n_size; // actual_rows is the number of rows assigned to process p
                                                  // multiplied by n_size to get number of elements
            if (p > 0) {
                senddispls[p] = senddispls[p - 1] + sendcounts[p - 1]; // displacement is cumulative
            }
        }
    }

    // Broadcast sendcounts and senddispls to all processes
    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(senddispls, size, MPI_INT, 0, MPI_COMM_WORLD);

    int global_row_start = my_rank * rows_per_proc;
    int remaining_rows = n_size - global_row_start;

    int my_rows;
    if (remaining_rows <= 0) {
        my_rows = 0;
    } else if (remaining_rows < rows_per_proc) {
        my_rows = remaining_rows;
    } else {
        my_rows = rows_per_proc;
    }

    // local buffers: each process gets its chunk of A and T (only actual rows, no padding)
    int *my_A = NULL;
    int *my_T = NULL;
    
    if (my_rows > 0) {
        my_A = malloc(my_rows * n_size * sizeof(int));
        my_T = malloc(my_rows * n_size * sizeof(int));
    }

    /* timing start **************************************/
    double start, end, local_elaps, global_elaps;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    /* timing start **************************************/

    /* MASTER: DATA DISTRIBUTION */
    MPI_Scatterv(
        A_raw, sendcounts, senddispls, MPI_INT, // send buffers
        my_A, (my_rows * n_size), MPI_INT,      // recv buffer
        0, MPI_COMM_WORLD
    );

    /* 1) neighbor ranks */
    int up   = (my_rank > 0)        ? my_rank - 1 : MPI_PROC_NULL;
    int down = (my_rank < size - 1) ? my_rank + 1 : MPI_PROC_NULL;
    int total_rows = my_rows + 2; // two ghost rows (above and below)

    /* Calcolo dell'indice globale della prima riga assegnata a questo processo */
    // (già calcolato sopra: global_row_start)

    /* Numero di righe effettivamente valide per questo processo */
    // (già calcolato sopra: my_rows)

    /* Calcolo dell'ultimo processo che contiene almeno una riga reale.
    Serve per gestire correttamente i bordi inferiori della matrice
    durante lo scambio delle ghost rows */
    int last_rank_with_data;

    if (n_size > 0) {
        /* Divisione intera: individua il rank dell'ultimo blocco
        che copre righe reali della matrice globale */
        last_rank_with_data = (n_size - 1) / rows_per_proc;
    }
    else {
        /* Caso limite: matrice vuota */
        last_rank_with_data = -1;
    }

    /* 2) add ghost rows above and below */
    int *my_A_plus_ghosts = malloc(total_rows * n_size * sizeof(int));
    int *upper_ghost = my_A_plus_ghosts;                          // row -1
    int *local_data  = my_A_plus_ghosts + n_size;                 // rows [0 ... my_rows-1]
    int *lower_ghost = my_A_plus_ghosts + (my_rows + 1) * n_size; // row + my_rows

    /* copy local block into the central part of the buffer */
    if (my_rows > 0) {
        memcpy(local_data, my_A, my_rows * n_size * sizeof(int));
    }

    /* 2) Halo exchange rows */
    // SSend / Recv version ********************************************************************
    int send_down_offset = (my_rows > 0 ? (my_rows - 1) * n_size : 0);
    if (my_rank > 0) {
        MPI_Ssend(local_data, n_size, MPI_INT, up, 100, MPI_COMM_WORLD);
        MPI_Recv(upper_ghost, n_size, MPI_INT, up, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (my_rank < size - 1) {
        MPI_Recv(lower_ghost, n_size, MPI_INT, down, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Ssend(local_data + send_down_offset, n_size, MPI_INT, down, 200, MPI_COMM_WORLD);
    }

    /* 3. Parallel processing (each process on its local domain) */
    for (int i = 0; i < my_rows; i++) { // handles only valid rows
        for (int j = 0; j < n_size; j++) {

            // initialize accumulators
            int sum = 0; // sum of the neighborhood values
            int count = 0;    // number of elements, for the mean

            // Rows index mapping nel buffer con ghost:
            // - my_A_plus_ghosts[0]              | upper ghost row
            // - my_A_plus_ghosts[1 ... my_rows]  | local rows
            // - my_A_plus_ghosts[my_rows + 1]    | lower ghost row

            /* 3.1 neighborhood  bounds */
            // vertical 
            int zmin = (i == 0 && my_rank == 0) ? 0 : -1; // top boundary
            int zmax = (i == my_rows - 1 && my_rank == last_rank_with_data) ? 0 : 1; // bottom boundary

            // horizontal
            int wmin = (j > 0) ? -1 : 0; // left boundary
            int wmax = (j < n_size - 1) ? 1 : 0; // right boundary

            /* 3.2 scan the 3x3 neighborhood */
            for (int dz = zmin; dz <= zmax; dz++) { // vertical offset
                int rowIndex = i + 1 + dz; // +1 to skip upper ghost

                for (int dw = wmin; dw <= wmax; dw++) { // horizontal offset
                    int colIndex = j + dw;

                    sum += my_A_plus_ghosts[rowIndex * n_size + colIndex];
                    count++;
                }
            }

            /* 3.5 thresholding operation */
            my_T[i * n_size + j] = (local_data[i * n_size + j] * count > sum) ? 1 : 0;
        }
    }

    free(my_A_plus_ghosts);

    /* MASTER: DATA GATHERING */
    MPI_Gatherv(
        my_T, (my_rows * n_size), MPI_INT,      // send buffer
        T_raw, sendcounts, senddispls, MPI_INT, // recv buffers
        0, MPI_COMM_WORLD
    );

    /* timing end **************************************/
    end = MPI_Wtime();
    local_elaps = end - start;
    /* timing end **************************************/

    MPI_Reduce(&local_elaps, &global_elaps, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        if (benchmark) {
            printf("%d,%d,%f\n", size, n_size, global_elaps);
        } else if (!quiet) {
            for (int i = 0; i < n_size; i++) {
                for (int j = 0; j < n_size ; j++) {
                    printf("%d ", T_raw[i * n_size + j]);
                }
                printf("\n");
            }
        }
        free(A_raw);
        free(T_raw);
    }

    if (my_rows > 0) {
        free(my_A);
        free(my_T);
    }

    free(sendcounts);
    free(senddispls);
    
    MPI_Finalize();

    return EXIT_SUCCESS;
}