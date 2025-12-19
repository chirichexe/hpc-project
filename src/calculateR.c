#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define N 4 // Dimensione della matrice N x N
#define TOT (N * N)

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Controllo sicurezza
    if(TOT % size != 0) {
        if(my_rank == 0) printf("Errore: TOT (%d) deve essere divisibile per size (%d)\n", TOT, size);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    printf("[Sono il processo %d di %d]\n", my_rank, size);

    // Buffer locali per ogni processo (porzioni della matrice)
    // Ogni processo riceve (N*N / size) elementi
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