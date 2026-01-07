#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define N 2000 // default matrix size
#define TOT (N * N)

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // check if TOT is divisible by size
    if(TOT % size != 0) {
        if(my_rank == 0) printf("Error: TOT (%d) must be divisible by size (%d)\n", TOT, size);

        // Terminate all processes
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    printf("[Process %d/%d]\n", my_rank, size);
    
    // Local buffers for each process (matrix portions)
    // Each process receives (N*N / size) elements
    int my_A[TOT/size], my_T[TOT/size];

    if (my_rank == 0)
    {
        int A[N][N], T[N][N]; // Matrici intere nel master
        int i, j;
        
        srand((unsigned int)time(NULL)); 
        
        // 1. Popolamento matrice A
        printf("Matrice A generata:\n");
        for(i=0; i<N; i++) {
            for(j=0; j<N; j++) {
                A[i][j] = rand() % 10;
                printf("%d ", A[i][j]);
            }
            printf("\n");
        }

        // Distribuzione: passiamo il puntatore alla matrice A
        MPI_Scatter(A, TOT/size, MPI_INT, my_A, TOT/size, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else
    {
        // I processi slave ricevono la loro parte
        MPI_Scatter(NULL, TOT/size, MPI_INT, my_A, TOT/size, MPI_INT, 0, MPI_COMM_WORLD);
    }

    // 2. Elaborazione Parallela
    // Ogni processo lavora sui propri elementi ricevuti
    for(int i=0; i < TOT/size; i++) {
        my_T[i] = my_A[i] * 2; // Esempio: genera T raddoppiando A
    }

    if (my_rank == 0) // Collettore
    {   
        int T[N][N]; 
        MPI_Gather(my_T, TOT/size, MPI_INT, T, TOT/size, MPI_INT, 0, MPI_COMM_WORLD);
        
        printf("\nRisultato Matrice T (elaborata):\n");
        for(int i=0; i<N; i++) {
            for(int j=0; j<N; j++) {
                printf("%d ", T[i][j]);
            }
            printf("\n");
        }
    }
    else
    {
        // Gli slave inviano il risultato al master
        MPI_Gather(my_T, TOT/size, MPI_INT, NULL, TOT/size, MPI_INT, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}