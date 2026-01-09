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
        if (pos == 0) { 
            if (val > 0) 
            n_size = (int)val; 
        }
        else if (pos == 1) { 
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* PARALLEL */
    MPI_Init(&argc, &argv);
    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int rows_per_proc = n_size / size; // matrix partitioning
    int *A_raw = NULL;                 // full matrix A (only on master)
    int *T_raw = NULL;                 // full matrix T (only on master)

    if (my_rank == 0) { /* MASTER */
        
        // allocates and initializes the full matrix A
        A_raw = malloc(n_size * n_size * sizeof(int));
        T_raw = malloc(n_size * n_size * sizeof(int));
        
        // seed of the random number generator
        srand(seed);

        // matrix A generation
        for (int i = 0; i < n_size * n_size; i++) 
            A_raw[i] = rand() % 10;
    }

    // local buffers: each process gets its portion of A and T
    int *my_A = malloc(rows_per_proc * n_size * sizeof(int));
    int *my_T = malloc(rows_per_proc * n_size * sizeof(int));

    /* timing start **************************************/
    double start, end, local_elaps, global_elaps;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    /* timing start **************************************/

    /* DATA DISTRIBUTION */
    MPI_Scatter(
        A_raw, rows_per_proc * n_size, MPI_INT, // send buffer: portion of A
        my_A, rows_per_proc * n_size, MPI_INT,  // recv buffer: each process receives its part of A
        0, MPI_COMM_WORLD
    );

    /* 1. neighbor ranks */
    int up   = (my_rank > 0)        ? my_rank - 1 : MPI_PROC_NULL; // if the rank is the first (0), there are not upper neighbors
    int down = (my_rank < size - 1) ? my_rank + 1 : MPI_PROC_NULL; // if the rank is the "last" (size-1), there are not lower neighbors
    int total_rows = rows_per_proc + 2; // always allocate 2 ghost rows (upper + lower)

    /* 2. add ghost rows above and below */
    int *my_A_plus_ghosts = malloc(total_rows * n_size * sizeof(int));
    int *upper_ghost = my_A_plus_ghosts;                                // row -1
    int *local_data  = my_A_plus_ghosts + n_size;                       // rows [0 ... rows_per_proc-1]
    int *lower_ghost = my_A_plus_ghosts + (rows_per_proc + 1) * n_size; // row + rows_per_proc

    /* copy local block into the central part of the buffer */
    memcpy(local_data, my_A, rows_per_proc * n_size * sizeof(int));+

    /* EXCHANGE ROWS */
    MPI_Sendrecv(
        local_data, n_size, MPI_INT, up, 0,    // send first local row upward
        lower_ghost, n_size, MPI_INT, down, 0, // receive lower ghost from below
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        local_data + (rows_per_proc - 1) * n_size, n_size, MPI_INT, down, 1, // send last local row downward
        upper_ghost, n_size, MPI_INT, up, 1,                                 // receive upper ghost from above
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    /* 3. Parallel processing (each process on its local domain) */
    for (int i = 0; i < rows_per_proc; i++) { // each process scans its local rows
        for (int j = 0; j < n_size; j++) {

            // initialize accumulators
            float sum = 0.0f; // sum of the neighborhood values
            int count = 0;    // number of elements, for the mean

            // Rows index mapping:
            // - my_A_plus_ghosts[0]                   | upper ghost row
            // - my_A_plus_ghosts[1 ... rows_per_proc] | local rows
            // - my_A_plus_ghosts[rows_per_proc + 1]   | lower ghost row

            /* 3.1 neighborhood  bounds */
            // vertical 
            int zmin = -1;
            int zmax =  1;

            if (i == 0 && my_rank == 0) // the cell is on the first row 
                                        // of the global matrix
                zmin = 0; // no upper neighbor globally

            if (i == rows_per_proc - 1 && my_rank == size - 1) // the cell is on the last row 
                                                               // of the global matrix
                zmax = 0; // no lower neighbor globally

            // horizontal
            int wmin = (j > 0) ? -1 : 0; // left boundary

            int wmax = (j < n_size - 1) ? 1 : 0; // right boundary

            /* 3.2 scan the 3x3 neighborhood */
            for (int dz = zmin; dz <= zmax; dz++) { // vertical offset
                int rowIndex = i + 1 + dz; // +1 due to the upper ghost row

                for (int dw = wmin; dw <= wmax; dw++) { // horizontal offset
                    int col = j + dw;

                    sum += my_A_plus_ghosts[rowIndex * n_size + col];
                    count++;
                }
            }

            /* 3.5 thresholding operation */
            my_T[i * n_size + j] = (local_data[i * n_size + j] * count > sum) ? 1 : 0;
        }
    }

    free(my_A_plus_ghosts);

    /* DATA GATHERING */
    MPI_Gather(
        my_T, rows_per_proc * n_size, MPI_INT,  // send buffer
        T_raw, rows_per_proc * n_size, MPI_INT, // recv buffer
        0, MPI_COMM_WORLD
    );

    /* timing end **************************************/
    end = MPI_Wtime();
    local_elaps = end - start;
    /* timing end **************************************/

    if (benchmark) {
        printf("%d,%f\n", my_rank, local_elaps);
    }

    MPI_Reduce(&local_elaps, &global_elaps, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        if (benchmark) {
            printf("MAX,%f\n", global_elaps);
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

    // confirmation message
    
    free(my_A);
    free(my_T);
    MPI_Finalize();

    //printf("Matrix bynarized with success.\n");
    return EXIT_SUCCESS;
}