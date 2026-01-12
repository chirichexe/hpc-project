#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#define N 2000 /* standard matrix size */

int main(int argc, char* argv[]) {
    
    // args parsing
    int n_size = N;
    int size = 0;
    int quiet = 0;
    int benchmark = 0;
    unsigned int seed = (unsigned int)time(NULL);

    if (argc < 2 || argc > 6) {
        fprintf(stderr, "Usage: %s <num_threads> [matrix_size] [seed] [-q|--quiet] [-b|--benchmark]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int pos = 0; // 0 = num threads, 1 = size, 2 = seed
    for (int k = 1; k < argc; k++) {

        // if quiet flag, no output
        if (strcmp(argv[k], "-q") == 0 || strcmp(argv[k], "--quiet") == 0) {
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
        if (pos == 0) { // first: num threads
            if (val > 0) 
                size = (int)val;  
        }
        if (pos == 1) { // second: MATRIX SIZE
            if (val > 0) 
                n_size = (int)val;  
        }
        else if (pos == 2) { // third: SEED
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* Values allocation */
    int *A_raw = malloc(n_size * n_size * sizeof(int));
    int *T_raw = malloc(n_size * n_size * sizeof(int));
    
    int (*A)[n_size] = (int (*)[n_size])A_raw; 
    int (*T)[n_size] = (int (*)[n_size])T_raw; 

    // initialize random seed
    srand(seed);

    /* Check memory allocation */
    if (!A || !T) {
        fprintf(stderr, "Error: Insufficient memory\n");
        return EXIT_FAILURE;
    }

    /* 1. matrix A generation */
    for (int i = 0; i < n_size; i++) {
        for (int j = 0; j < n_size; j++) {
            A[i][j] = rand() % 10;
        }
    }

    double end, start_time = 0.0;
    if (benchmark) {
        start_time = omp_get_wtime();
    }

    /* 2. binarization processing (OpenMP) */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n_size; i++) {
        for (int j = 0; j < n_size; j++) {

            // 2.1 calculate the mean of the neighborhood
            float sum = 0;
            int count = 0;

            int zmin = (i > 0) ? i - 1 : i;
            int zmax = (i < n_size - 1) ? i + 1 : i;
            int wmin = (j > 0) ? j - 1 : j;
            int wmax = (j < n_size - 1) ? j + 1 : j;

            for (int z = zmin; z <= zmax; z++) {
                for (int w = wmin; w <= wmax; w++) {
                    sum += A[z][w];
                    count++;
                }
            }
            
            // 2.2 calculate mean
            T[i][j] = (A[i][j] * count > sum) ? 1 : 0;
        }
    }

    /* 3. timing end and print */
    if (benchmark) {
        end = omp_get_wtime();
        double elapsed = end - start_time;
        printf("%d,%d,%f\n", size, n_size, elapsed);
    }

    /* 4. matrix print if not in quiet mode */
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