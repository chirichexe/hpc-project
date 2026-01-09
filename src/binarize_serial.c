#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define N 2000 /* standard matrix size */
#define TOT (N * N)

int main( int argc, char* argv[] ) {
    
    // args parsing
    int n_size = N;
    int quiet = 0;
    unsigned int seed = (unsigned int)time(NULL); // default: TIME

    int pos = 0; // 0 = size, 1 = seed
    for (int k = 1; k < argc; k++) {

        // if quiet flag, no output
        if (strcmp(argv[k], "--q") == 0) {
            quiet = 1;
            continue;
        }
        char *end = NULL;
        long val = strtol(argv[k], &end, 10);
        if (*end != '\0') continue; // not a valid number, skip

        if (pos == 0) { // matrix size
            if (val > 0) n_size = (int)val;
        } else if (pos == 1) { // seed
            seed = (unsigned int)val;
        }
        pos++;
    }

    /* Values allocation */
    int *A_raw = malloc(n_size * n_size * sizeof(int));
    int *T_raw = malloc(n_size * n_size * sizeof(int));
    
    // matrix as 2D arrays
    int (*A)[n_size] = (int (*)[n_size])A_raw; 
    int (*T)[n_size] = (int (*)[n_size])T_raw; 

    int i, j, count;
    float sum, mij;

    // initialize random seed
    srand(seed);

    /* Check memory allocation */
    if (!A || !T) {
        fprintf(stderr, "Error: Insufficient memory\n");
        return EXIT_FAILURE;
    }

    /* 1. matrix A generation */
    for (i = 0; i < n_size; i++) {
        for (j = 0; j < n_size; j++) {
            A[i][j] = rand() % 10;
        }
    }

    /* 2. Serial processing */
    for (i = 0; i < n_size; i++) {
        for (j = 0; j < n_size; j++) {

            // 2.1 calculate the mean of the neighborhood
            sum = 0;
            count = 0;

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

    if (!quiet) {
        for (i = 0; i < n_size; i++) {
            for (j = 0; j < n_size; j++) {
                printf("%d ", T[i][j]);
            }
            printf("\n");
        }
    }
    
    // confirmation message
    //printf("Matrix bynarized with success.\n");

    // free memory
    free(A);
    free(T);

    return EXIT_SUCCESS;
}