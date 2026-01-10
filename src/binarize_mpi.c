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
 
    int pos = 0; // 0 = size, 1 = seed
    for (int k = 1; k < argc; k++) {

        // if quiet flag, no output
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
        if (pos == 0) { // first: MATRIX SIZE
            if (val > 0) 
                n_size = (int)val;  
        }
        else if (pos == 1) { // second: SEED
            seed = (unsigned int)val; 
        }
        pos++;
    }

    /* PARALLEL */
    MPI_Init(&argc, &argv);
    int size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Number of rows per process (simplified distribution)
    int base_rows = n_size / size;
    int extra_rows = n_size % size;

    int *A_raw = NULL; // matrix A (only on master)
    int *T_raw = NULL; // matrix T (only on master)

    /* Data structures for Scatterv */
    // it says me how many elements each process will receive
    int *sendcounts = malloc(size * sizeof(int));
    
    // it says me the displacement (in elements) from the beginning of A_raw for each process
    int *senddispls = malloc(size * sizeof(int));

    if (my_rank == 0) { /* MASTER */
        
        // allocates and initializes the full matrix A and T
        A_raw = malloc(n_size * n_size * sizeof(int));
        T_raw = malloc(n_size * n_size * sizeof(int));
        
        // seed of the random number generator
        srand(seed);

        // matrix A generation 
        for (int i = 0; i < n_size; i++) {
            for (int j = 0; j < n_size; j++) {
                A_raw[i * n_size + j] = rand() % 10;
            }
        }

        /* Calculation of sendcounts and senddispls */
        int current_displ = 0;
        for (int p = 0; p < size; p++) {
            // distribute extra rows to the first 'extra_rows' processes
            int actual_rows = base_rows + (p < extra_rows ? 1 : 0);
            
            sendcounts[p] = actual_rows * n_size; 
            senddispls[p] = current_displ;
            current_displ += sendcounts[p];
        }
    }

    // Broadcast sendcounts and senddispls to all processes
    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(senddispls, size, MPI_INT, 0, MPI_COMM_WORLD);

    /* EACH PROCESS */
    // the number of rows assigned to this process
    int my_rows = sendcounts[my_rank] / n_size;

    // local buffers: each process gets its chunk of A and T (only actual rows, no padding)
    int *my_A = NULL;
    int *my_T = NULL;
    
    if (my_rows > 0) {
        my_A = malloc( sendcounts[my_rank] * sizeof(int));
        my_T = malloc( sendcounts[my_rank] * sizeof(int));
    }

    /* timing start **************************************/
    double start, end, local_elaps, global_elaps;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    /* timing start **************************************/

    /* MASTER: DATA DISTRIBUTION */
    MPI_Scatterv(
        A_raw, sendcounts, senddispls, MPI_INT,  // send buffers
        my_A, (  sendcounts[my_rank] ), MPI_INT, // recv buffer
        0, MPI_COMM_WORLD
    );

    /* 1) neighbor ranks */
    int up   = (my_rank > 0)        ? my_rank - 1 : MPI_PROC_NULL;
    int down = (my_rank < size - 1) ? my_rank + 1 : MPI_PROC_NULL;
    int total_rows = my_rows + 2; // two ghost rows (above and below)

    // the last process that contains data
    int last_rank_with_data = size - 1;
    while (last_rank_with_data >= 0 && sendcounts[last_rank_with_data] == 0) {
        last_rank_with_data--;
    }

    /* 2) add ghost rows above and below */
    int *my_A_plus_ghosts = calloc(total_rows * n_size, sizeof(int));
    int *upper_ghost = my_A_plus_ghosts;                          // row -1
    int *local_data  = my_A_plus_ghosts + n_size;                 // rows [0 ... my_rows-1]
    int *lower_ghost = my_A_plus_ghosts + (my_rows + 1) * n_size; // row + my_rows

    /* copy local block into the central part of the buffer */
    if (my_rows > 0) {
        memcpy(local_data, my_A, my_rows * n_size * sizeof(int));
    }

    /* 2) Halo exchange rows 
    // SSend / Recv version ********************************************************************
    if (my_rank > 0 && my_rows > 0) {
        MPI_Ssend(local_data, n_size, MPI_INT, up, 100, MPI_COMM_WORLD);
        MPI_Recv(upper_ghost, n_size, MPI_INT, up, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    int send_down_offset = (my_rows > 0 ? (my_rows - 1) * n_size : 0);

    if (my_rank < size - 1 && my_rows > 0) {
        if (sendcounts[down] > 0) {
            MPI_Recv(lower_ghost, n_size, MPI_INT, down, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Ssend(local_data + send_down_offset, n_size, MPI_INT, down, 200, MPI_COMM_WORLD);
        }
    }*/

    /* 2) Halo exchange rows (ASYNCHRONOUS version) */
    MPI_Request requests[4];
    int req_count = 0;
    int send_down_offset = (my_rows > 0 ? (my_rows - 1) * n_size : 0);

    // Prepariamo le ricezioni (Irecv)
    if (my_rank > 0 && my_rows > 0) {
        MPI_Irecv(upper_ghost, n_size, MPI_INT, up, 200, MPI_COMM_WORLD, &requests[req_count++]);
    }
    if (my_rank < size - 1 && my_rows > 0 && sendcounts[down] > 0) {
        MPI_Irecv(lower_ghost, n_size, MPI_INT, down, 100, MPI_COMM_WORLD, &requests[req_count++]);
    }

    // Prepariamo gli invii (Isend)
    if (my_rank > 0 && my_rows > 0) {
        MPI_Isend(local_data, n_size, MPI_INT, up, 100, MPI_COMM_WORLD, &requests[req_count++]);
    }
    if (my_rank < size - 1 && my_rows > 0 && sendcounts[down] > 0) {
        MPI_Isend(local_data + send_down_offset, n_size, MPI_INT, down, 200, MPI_COMM_WORLD, &requests[req_count++]);
    }

    // Attendiamo che tutti gli scambi siano completati prima di procedere al calcolo
    if (req_count > 0) {
        MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);
    }

    /* 3. Parallel processing (each process on its local domain) */
    for (int i = 0; i < my_rows; i++) { // handles only valid rows
        for (int j = 0; j < n_size; j++) {

            // initialize accumulators
            int sum = 0; // sum of the neighborhood values
            int count = 0;    // number of elements, for the mean

            /* 3.1 neighborhood  bounds */
            // vertical 
            int zmin = (i == 0 && my_rank == 0) ? 0 : -1; // top boundary
            int zmax = (i == my_rows - 1 && my_rank == last_rank_with_data) ? 0 : 1; // bottom boundary

            // horizontal
            int wmin = (j > 0) ? -1 : 0; // left boundary
            int wmax = (j < n_size - 1) ? 1 : 0; // right boundary

            /* 3.2 scan the 3x3 neighborhood */
            for (int dz = zmin; dz <= zmax; dz++) { // vertical offset
                int rowIndex = i + 1 + dz; // +1 to skip upper ghost

                for (int dw = wmin; dw <= wmax; dw++) { // horizontal offset
                    int colIndex = j + dw;

                    sum += my_A_plus_ghosts[rowIndex * n_size + colIndex];
                    count++;
                }
            }

            /* 3.5 thresholding operation */
            my_T[i * n_size + j] = (local_data[i * n_size + j] * count > sum) ? 1 : 0;
        }
    }

    free(my_A_plus_ghosts);

    /* MASTER: DATA GATHERING */
    MPI_Gatherv(
        my_T, (my_rows * n_size), MPI_INT,      // send buffer
        T_raw, sendcounts, senddispls, MPI_INT, // recv buffers
        0, MPI_COMM_WORLD
    );

    /* timing end **************************************/
    end = MPI_Wtime();
    local_elaps = end - start;
    /* timing end **************************************/

    MPI_Reduce(&local_elaps, &global_elaps, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        if (benchmark) {
            printf("%d,%d,%f\n", size, n_size, global_elaps);
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

    if (my_rows > 0) {
        free(my_A);
        free(my_T);
    }

    free(sendcounts);
    free(senddispls);
    
    MPI_Finalize();

    return EXIT_SUCCESS;
}