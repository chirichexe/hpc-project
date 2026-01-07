#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define N 2000 /* standard matrix size */
#define TOT (N * N)

int main( int argc, char* argv[] ) {
    
    /* Values allocation */
    int (*A)[N] = malloc(sizeof(int[N][N])); // A = starting matrixs
    int (*T)[N] = malloc(sizeof(int[N][N])); // T = bynarized matrix
    int i, j;
    float sum, mij;
    int count;

    // initialize random seed
    srand((unsigned int)time(NULL));

    /* Check memory allocation */
    if (A == NULL || T == NULL) {
        fprintf(stderr, "Error: Insufficient memory\n");
        return EXIT_FAILURE;
    }

    /* 1. matrix A generation */
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
        }
    }

    /* 2. Serial processing */
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {

            // 2.1 calculate the mean of the neighborhood
            sum = 0;
            count = 0;
            
            for (int z = i - 1; z <= i + 1; z++) {
                for (int w = j - 1; w <= j + 1; w++) {
                    
                    // check for boundaries
                    if (z >= 0 && z < N && w >= 0 && w < N) {
                        sum += A[z][w];
                        count++;
                    }
                }
            }
            
            // 2.2 calculate mean
            mij = sum / count; 

            // 2.3 bynarization
            if (A[i][j] > mij) {
                T[i][j] = 1;
            } else {
                T[i][j] = 0;
            }
        }
    }

    // 3. Print Result
    //if (strcmp(argv[2], "--verbose") == 0) {
        printf("Generated Matrix A:\n");
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                printf("%d ", T[i][j]);
            }
            printf("\n");
        }
    //}
    
    // confirmation message
    printf("Matrix bynarized with success.\n");

    // free memory
    free(A);
    free(T);

    return EXIT_SUCCESS;
}