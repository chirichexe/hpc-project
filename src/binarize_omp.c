#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#define N 2000 /* standard matrix size */

int main(int argc, char* argv[]) {
    
    /* args */
    // matrix size
    int n_size = N;
    
    // number of threads
    int size = 0;

    // quiet mode
    int quiet = 0;

    // benchmark mode
    int benchmark = 0;

    // seed of the random number generator
    unsigned int seed = (unsigned int)time(NULL);

    /* args check and parsing */
    if (argc < 2 || argc > 6) {
        fprintf(stderr, "Usage: %s <num_threads> [matrix_size] [seed] [-q|--quiet] [-b|--benchmark]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int pos = 0; // 0 = num threads, 1 = size, 2 = seed
    for (int k = 1; k < argc; k++) {

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
        if (pos == 0) {         // first: NUM THREADS
            if (val > 0) 
                size = (int)val;  
        }
        if (pos == 1) {         // second: MATRIX SIZE
            if (val > 0) 
                n_size = (int)val;  
        }
        else if (pos == 2) {    // third: SEED
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* Values allocation */
    // input matrix A
    int *A = malloc(n_size * n_size * sizeof *A);
    
    // output matrix T
    int *T = malloc(n_size * n_size * sizeof *T);

    // Check memory allocation
    if (!A || !T) {
        fprintf(stderr, "Error: Insufficient memory\n");
        return EXIT_FAILURE;
    }

    // initialize random seed
    srand(seed);

    /* Check memory allocation */
    if (!A || !T) {
        fprintf(stderr, "Error: Insufficient memory\n");
        return EXIT_FAILURE;
    }

    /* 1. Matrix A generation */
    for (int i = 0; i < n_size; i++) {
        for (int j = 0; j < n_size; j++) {
            A[i * n_size + j] = rand() % 10;
        }
    }

    /* TIMING START **********************************************/
    double end, start_time = 0.0;
    if (benchmark) {
        start_time = omp_get_wtime();
    }
    /* TIMING START **********************************************/


    /* 2. binarization processing (OpenMP) */
    #pragma omp parallel for num_threads(size) schedule(static) 
    for (int i = 0; i < n_size; i++) {
        for (int j = 0; j < n_size; j++) {

            // 2.1 Calculate the mean of the neighborhood
            float sum = 0;
            int count = 0;

            int zmin = (i > 0) ? i - 1 : i;
            int zmax = (i < n_size - 1) ? i + 1 : i;
            int wmin = (j > 0) ? j - 1 : j;
            int wmax = (j < n_size - 1) ? j + 1 : j;

            for (int z = zmin; z <= zmax; z++) {
                for (int w = wmin; w <= wmax; w++) {
                    sum += A[z * n_size + w];
                    count++;
                }
            }
            
            // 2.2 Calculate mean
            T[i * n_size + j] = (A[i * n_size + j] * count > sum) ? 1 : 0;
        }
    }

    /* TIMING END **********************************************/
    if (benchmark) {
        end = omp_get_wtime();
        double elapsed = end - start_time;
        printf("%d,%d,%f\n", size, n_size, elapsed);
    }
    /* TIMING END **********************************************/


    /* 3. Matrix print if not in quiet mode */
    if (!quiet) {
        for (int i = 0; i < n_size; i++) {
            for (int j = 0; j < n_size; j++) {
                printf("%d ", T[i * n_size + j]);
            }
            printf("\n");
        }
    }

    /* 4. Free memory */
    free(A);
    free(T);

    return EXIT_SUCCESS;
}