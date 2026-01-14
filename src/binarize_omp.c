#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#define DEFAULT_N 2000 /* standard matrix num_threads */

int main(int argc, char* argv[]) {
    
    /* args */
    // matrix num_threads
    int N = DEFAULT_N; 
    
    // number of threads
    int num_threads = 0;

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
    
    int pos = 0; // 0 = num threads, 1 = num_threads, 2 = seed
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
                num_threads = (int)val;  
        }
        if (pos == 1) {         // second: MATRIX SIZE
            if (val > 0) 
                N = (int)val;  
        }
        else if (pos == 2) {    // third: SEED
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* Values allocation */
    // input matrix A
    int *A = malloc(N * N * sizeof *A);
    
    // output matrix T
    int *T = malloc(N * N * sizeof *T);

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
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i * N + j] = rand() % 10;
        }
    }

    /* TIMING START **********************************************/
    double end, start_time = 0.0;
    if (benchmark) {
        start_time = omp_get_wtime();
    }
    /* TIMING START **********************************************/


    // for variables declaration
    int i, j, z, w, count, zmin, zmax, wmin, wmax;
    float sum;

    /* 2. binarization processing (OpenMP) */
    #pragma omp parallel for num_threads(num_threads) schedule(static) \
    shared(A, T, N) private(i, j, sum, count, zmin, zmax, wmin, wmax, z, w)
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {

            // 2.1 calculate the mean of the neighborhood
            sum = 0;
            count = 0;

            zmin = (i > 0) ? i - 1 : i;
            zmax = (i < N - 1) ? i + 1 : i;
            wmin = (j > 0) ? j - 1 : j;
            wmax = (j < N - 1) ? j + 1 : j;

            for (z = zmin; z <= zmax; z++) {
                for (w = wmin; w <= wmax; w++) {
                    sum += A[z * N + w];
                    count++;
                }
            }
            
            // 2.2 Calculate mean
            T[i * N + j] = (A[i * N + j] * count > sum) ? 1 : 0;
        }
    }

    /* TIMING END **********************************************/
    if (benchmark) {
        end = omp_get_wtime();
        double elapsed = end - start_time;
        printf("%d,%d,%f\n", num_threads, N, elapsed);
    }
    /* TIMING END **********************************************/


    /* 3. Matrix print if not in quiet mode */
    if (!quiet) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                printf("%d ", T[i * N + j]);
            }
            printf("\n");
        }
    }

    /* 4. Free memory */
    free(A);
    free(T);

    return EXIT_SUCCESS;
}
