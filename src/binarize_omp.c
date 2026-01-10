#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#define N 2000 /* standard matrix size */
#define TOT (N * N)

int main(int argc, char* argv[]) {
    
    // args parsing
    int n_size = N;
    int quiet = 0;
    unsigned int seed = (unsigned int)time(NULL);

    int pos = 0; // 0 = size, 1 = seed
    for (int k = 1; k < argc; k++) {

        // if quiet flag, no output
        if (strcmp(argv[k], "--q") == 0) {
            quiet = 1;
            continue;
        }
        char *end = NULL;
        long val = strtol(argv[k], &end, 10);
        if (*end != '\0') continue; 

        if (pos == 0) { // first: MATRIX SIZE
            if (val > 0) 
                n_size = (int)val;
        } 
        else if (pos == 1) { // second: SEED
            seed = (unsigned int)val;
        }
        pos++;
    }

    /* Values allocation */
    int *A_raw = malloc(n_size * n_size * sizeof(int));
    int *T_raw = malloc(n_size * n_size * sizeof(int));
    
    int (*A)[n_size] = (int (*)[n_size])A_raw; 
    int (*T)[n_size] = (int (*)[n_size])T_raw; 

    /* Check memory allocation */
    if (!A || !T) {
        fprintf(stderr, "Error: Insufficient memory\n");
        return EXIT_FAILURE;
    }

    /* 1. Matrix A generation - parallelized with OpenMP */
    #pragma omp parallel
    {
        // Each thread gets its own random seed to avoid contention
        unsigned int thread_seed = seed + omp_get_thread_num();
        
        #pragma omp for collapse(2) schedule(static)
        for (int i = 0; i < n_size; i++) {
            for (int j = 0; j < n_size; j++) {
                A[i][j] = rand_r(&thread_seed) % 10;
            }
        }
    }

    /* 2. Binarization processing - parallelized with OpenMP */
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < n_size; i++) {
        for (int j = 0; j < n_size; j++) {

            // 2.1 Calculate the mean of the neighborhood
            float sum = 0;
            int count = 0;

            // Determine neighborhood bounds (handling borders)
            int zmin = (i > 0) ? i - 1 : i;
            int zmax = (i < n_size - 1) ? i + 1 : i;
            int wmin = (j > 0) ? j - 1 : j;
            int wmax = (j < n_size - 1) ? j + 1 : j;

            // Sum neighborhood values
            for (int z = zmin; z <= zmax; z++) {
                for (int w = wmin; w <= wmax; w++) {
                    sum += A[z][w];
                    count++;
                }
            }
            
            // 2.2 Binarize: compare element with mean
            T[i][j] = (A[i][j] * count > sum) ? 1 : 0;
        }
    }

    /* 3. Output result if not in quiet mode */
    if (!quiet) {
        for (int i = 0; i < n_size; i++) {
            for (int j = 0; j < n_size; j++) {
                printf("%d ", T[i][j]);
            }
            printf("\n");
        }
    }

    // Free memory
    free(A);
    free(T);

    return EXIT_SUCCESS;
}