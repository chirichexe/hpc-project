#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define DEFAULT_N 2000 /* standard matrix size */

int main( int argc, char* argv[] ) {
    
    /* args */
    // matrix size
    int N = DEFAULT_N;
    
    // seed of the random number generator
    unsigned int seed = (unsigned int)time(NULL);

    // quiet mode
    int quiet = 0;

    /* args check and parsing */
    if (argc > 4) {
        fprintf(stderr, "Usage: %s [N] [seed] [-q|--quiet]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pos = 0; // 0 = size, 1 = seed
    for (int k = 1; k < argc; k++) {

        if (strcmp(argv[k], "-q") == 0 || strcmp(argv[k], "--quiet") == 0) {
            quiet = 1;
            continue;
        }
        char *end = NULL;
        long val = strtol(argv[k], &end, 10);
        if (*end != '\0') continue; 

        if (pos == 0) {         // first: MATRIX SIZE
            if (val > 0) 
                N = (int)val;
        } 
        else if (pos == 1) {    // second: SEED
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

    // variables
    int i, j, count, zmin, zmax, wmin, wmax, z, w;
    float sum;

    // initialize random seed
    srand(seed);

    /* 1. Matrix A generation */
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i * N + j] = rand() % 10;
        }
    }

    /* 2. Binarization processing (serial) */
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {

            // 2.1 Calculate the mean of the neighborhood
            sum = 0;
            count = 0;

            zmin = (i > 0) ? i - 1 : i;
            zmax = (i < N - 1) ? i + 1 : i;
            wmin = (j > 0) ? j - 1 : j;
            wmax = (j < N - 1) ? j + 1 : j;

            for ( z = zmin; z <= zmax; z++) {
                for ( w = wmin; w <= wmax; w++) {
                    sum += A[z * N + w];
                    count++;
                }
            }
            
            // 2.2 Calculate mean
            T[i * N + j] =
                (A[i * N + j] * count > sum) ? 1 : 0;
        }
    }

    /* 3. Matrix print if not in quiet mode */
    if (!quiet) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
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
