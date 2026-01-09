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

    int pos = 0;
    for (int k = 1; k < argc; k++) {
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
        else if (pos == 1) { // second: RANDOM SEED
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* PARALLEL */
    MPI_Init(&argc, &argv);
    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Numero di righe per processo con padding (ceil division)
    int rows_per_proc = (n_size + size - 1) / size;
    int padded_rows_total = rows_per_proc * size; // righe totali con padding

    int *A_raw = NULL; // full matrix A (only on master, con padding)
    int *T_raw = NULL; // full matrix T (only on master, con padding)

    if (my_rank == 0) { /* MASTER */
        
        // allocates and initializes the full (padded) matrix A e T
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
    }

    // local buffers: each process gets its chunk of A and T
    int *my_A = malloc(rows_per_proc * n_size * sizeof(int));
    int *my_T = malloc(rows_per_proc * n_size * sizeof(int));

    /* timing start **************************************/
    double start, end, local_elaps, global_elaps;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    /* timing start **************************************/

    /* MASTER: DATA DISTRIBUTION */
    MPI_Scatter(
        A_raw, rows_per_proc * n_size, MPI_INT, // send buffer
        my_A, rows_per_proc * n_size, MPI_INT,  // recv buffer
        0, MPI_COMM_WORLD
    );

    /* 1) neighbor ranks */
    int up   = (my_rank > 0)        ? my_rank - 1 : MPI_PROC_NULL;
    int down = (my_rank < size - 1) ? my_rank + 1 : MPI_PROC_NULL;
    int total_rows = rows_per_proc + 2; // two ghost rows (above and below)

    // ------------------------------------------------------------------------------------- ??
    /* Calcolo dell'indice globale della prima riga assegnata a questo processo */
    int global_row_start = my_rank * rows_per_proc;

    /* Numero di righe ancora disponibili nella matrice globale
    a partire da global_row_start */
    int remaining_rows = n_size - global_row_start;

    /* Numero di righe effettivamente valide per questo processo
    (le eventuali righe in eccesso sono padding) */
    int my_rows;

    if (remaining_rows <= 0) {
        /* Questo processo non riceve alcuna riga reale:
        tutto il suo blocco è padding */
        my_rows = 0;
    }
    else if (remaining_rows < rows_per_proc) {
        /* Questo processo riceve solo una parte del blocco:
        le prime 'remaining_rows' righe sono valide,
        le restanti sono padding */
        my_rows = remaining_rows;
    }
    else {
        /* Questo processo riceve un blocco completo di righe valide */
        my_rows = rows_per_proc;
    }

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
    // ------------------------------------------------------------------------------------- ??

    /* 2) add ghost rows above and below */
    int *my_A_plus_ghosts = malloc(total_rows * n_size * sizeof(int));
    int *upper_ghost = my_A_plus_ghosts;                                // row -1
    int *local_data  = my_A_plus_ghosts + n_size;                       // rows [0 ... rows_per_proc-1] (include padding)
    int *lower_ghost = my_A_plus_ghosts + (rows_per_proc + 1) * n_size; // row + rows_per_proc

    /* copy local block into the central part of the buffer */
    memcpy(local_data, my_A, rows_per_proc * n_size * sizeof(int)); // (fix: rimosso '+' errato)

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

    /*
    if (my_rank % 2 == 0) {
        // I rank pari inviano prima
        if (down != MPI_PROC_NULL) MPI_Ssend(local_data + send_down_idx, n_size, MPI_INT, down, 100, MPI_COMM_WORLD);
        if (up != MPI_PROC_NULL)   MPI_Recv(upper_ghost, n_size, MPI_INT, up, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        // I rank dispari ricevono prima
        if (up != MPI_PROC_NULL)   MPI_Recv(upper_ghost, n_size, MPI_INT, up, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (down != MPI_PROC_NULL) MPI_Ssend(local_data + send_down_idx, n_size, MPI_INT, down, 100, MPI_COMM_WORLD);
    }

    // --- FASE 2: Invio verso l'alto, ricezione dal basso ---
    if (my_rank % 2 == 0) {
        if (up != MPI_PROC_NULL)   MPI_Ssend(local_data, n_size, MPI_INT, up, 200, MPI_COMM_WORLD);
        if (down != MPI_PROC_NULL) MPI_Recv(lower_ghost, n_size, MPI_INT, down, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        if (down != MPI_PROC_NULL) MPI_Recv(lower_ghost, n_size, MPI_INT, down, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (up != MPI_PROC_NULL)   MPI_Ssend(local_data, n_size, MPI_INT, up, 200, MPI_COMM_WORLD);
    }
    */

    // SendRecv version ********************************************************************
    /*
    // 2.1) Simultaneous send/recv to/from 'up' and 'down'
    MPI_Sendrecv(
        local_data, n_size, MPI_INT, up, 0 ,
        lower_ghost, n_size, MPI_INT, down, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );
    MPI_Sendrecv(
        local_data + (rows_per_proc - 1) * n_size, n_size, MPI_INT, down, 1,
        upper_ghost, n_size, MPI_INT, up, 1,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );
    */

    /* 3. Parallel processing (each process on its local domain) */
    for (int i = 0; i < my_rows; i++) { // handles only valid rows
        for (int j = 0; j < n_size; j++) {

            // initialize accumulators
            float sum = 0.0f; // sum of the neighborhood values
            int count = 0;    // number of elements, for the mean

            // Rows index mapping nel buffer con ghost:
            // - my_A_plus_ghosts[0]                   | upper ghost row
            // - my_A_plus_ghosts[1 ... rows_per_proc] | local rows (con padding in coda)
            // - my_A_plus_ghosts[rows_per_proc + 1]   | lower ghost row

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
    MPI_Gather(
        my_T, rows_per_proc * n_size, MPI_INT,  // send buffer (include padding)
        T_raw, rows_per_proc * n_size, MPI_INT, // recv buffer (size * rows_per_proc righe)
        0, MPI_COMM_WORLD
    );

    /* timing end **************************************/
    end = MPI_Wtime();
    local_elaps = end - start;
    /* timing end **************************************/

    /*
    if (benchmark) {
        printf("%d,%f\n", my_rank, local_elaps);
    }
    */

    MPI_Reduce(&local_elaps, &global_elaps, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        if (benchmark) {
            printf("%d,%d,%f\n", size, n_size, global_elaps);
            //printf("MAX,%f\n", global_elaps);
        } else if (!quiet) {
            // Stampa solo le prime n_size righe (ignora il padding)
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

    free(my_A);
    free(my_T);
    MPI_Finalize();

    return EXIT_SUCCESS;
}