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

    // check if n_size is divisible by size
    if (n_size % size != 0) {
        if (my_rank == 0) printf("Error: n_size must be divisible by the number of processes.\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // this operation is executed by all processes
    int rows_per_proc = n_size / size;
    int *A_raw = NULL;
    int *T_raw = NULL;

    if (my_rank == 0) { /* MASTER */
        // only master allocates and initializes the full matrix A
        A_raw = malloc(n_size * n_size * sizeof(int));
        T_raw = malloc(n_size * n_size * sizeof(int));
        srand(seed);
        for (int i = 0; i < n_size * n_size; i++) A_raw[i] = rand() % 10;
    }

    // local buffers: each process gets its portion of A and T
    int *my_A = malloc(rows_per_proc * n_size * sizeof(int));
    int *my_T = malloc(rows_per_proc * n_size * sizeof(int));

    /* TIMING START */
    double start, end, local_elaps, global_elaps;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    /* DATA DISTRIBUTION */
    MPI_Scatter(
        A_raw, rows_per_proc * n_size, MPI_INT,
        my_A, rows_per_proc * n_size, MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    /* EVERY PROCESS */
    for (int i = 0; i < rows_per_proc; i++) {
        for (int j = 0; j < n_size; j++) {
            float sum = 0;
            int count = 0;

            // local 3x3 loop
            for (int z = i - 1; z <= i + 1; z++) {
                for (int w = j - 1; w <= j + 1; w++) {
                    
                    // check bounds within local data
                    if (z >= 0 && z < rows_per_proc && w >= 0 && w < n_size) {
                        sum += my_A[z * n_size + w];
                        count++;
                    }
                }
            }
            
            my_T[i * n_size + j] = (my_A[i * n_size + j] * count > sum) ? 1 : 0;
        }
    }

    /* DATA GATHERING */
    MPI_Gather(
        my_T, rows_per_proc * n_size, MPI_INT,
        T_raw, rows_per_proc * n_size, MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    /* TIMING END */
    end = MPI_Wtime();
    local_elaps = end - start;

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
    printf("Matrix bynarized with success.\n");

    free(my_A);
    free(my_T);
    MPI_Finalize();
    return EXIT_SUCCESS;
}