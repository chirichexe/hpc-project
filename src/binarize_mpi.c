#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

#define DEFAULT_N 2000 /* standard matrix num_proc */
#define UP_TAG 100
#define DOWN_TAG 200

int main(int argc, char* argv[]) {

    /* args */
    // matrix num_proc
    int N = DEFAULT_N;

    // quiet mode
    int quiet = 0;

    // benchmark mode
    int benchmark = 0;

    // halo exchange mode
    int exchange_mode = 0; // 0 = ssend/recv, 1 = isend/irecv, 2 = sendrecv
    
    // seed of the random number generator
    unsigned int seed = (unsigned int)time(NULL);
 
    /* args check and parsing */
    if (argc > 6) {
        printf("Usage: %s [matriz_size] [seed] [exchange_mode] [-q|--quiet] [-b|--benchmark]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pos = 0; // 0 = num_proc, 1 = seed, 2 =  exchange mode
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
        if (pos == 0) {         // first: MATRIX SIZE
            if (val > 0) 
                N = (int)val;  
        }
        else if (pos == 1) {    // second: SEED
            seed = (unsigned int)val;
        } 
        else if (pos == 2) {    // third: HALO MODE
            if (val >=0 && val <=2)
                exchange_mode = (int)val;
        }
        pos++;
    }

    /* Parallel Section */
    MPI_Init(&argc, &argv);
    int num_proc, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* 1. Data distribution */
    // the minimum number of rows assigned to each process
    int base_rows = N / num_proc;

    // extra rows to distribute among the first 'extra_rows' processes
    int extra_rows = N % num_proc;

    int *A = NULL; // matrix A (only on master)
    int *T = NULL; // matrix T (only on master)

    /* Data structures for Scatterv */
    // it says me how many elements each process will receive
    int *sendcounts = malloc(num_proc * sizeof(int));
    
    // it says me the displacement (in elements) from the beginning of A for each process
    int *senddispls = malloc(num_proc * sizeof(int));

    if (my_rank == 0) { /* MASTER */
        
        // allocates and initializes the full matrix A and T
        A = malloc(N * N * sizeof(int));
        T = malloc(N * N * sizeof(int));
        
        // initialize the seed of the random number generator
        srand(seed);

        // matrix A generation 
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i * N + j] = rand() % 10;
            }
        }

        /* Calculation of sendcounts and senddispls */
        int current_displ = 0;
        for (int p = 0; p < num_proc; p++) {
            // distribute extra rows to the first 'extra_rows' processes
            int actual_rows = base_rows + (p < extra_rows ? 1 : 0);
            
            sendcounts[p] = actual_rows * N; 
            senddispls[p] = current_displ;
            current_displ += sendcounts[p];
        }
    }

    // 2. Broadcast sendcounts and senddispls to all processes
    MPI_Bcast(sendcounts, num_proc, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(senddispls, num_proc, MPI_INT, 0, MPI_COMM_WORLD);

    /* EACH PROCESS */
    // the number of rows assigned to this process
    int my_rows = sendcounts[my_rank] / N;

    // local buffers: each process gets its chunk of A and T (only actual rows, no padding)
    int *my_T = NULL;
    
    if (my_rows > 0) {
        my_T = malloc( sendcounts[my_rank] * sizeof(int));
    }

    /* 1) neighbor ranks */
    int up   = (my_rank > 0)        ? my_rank - 1 : MPI_PROC_NULL;
    int down = (my_rank < num_proc - 1) ? my_rank + 1 : MPI_PROC_NULL;
    
    /* 2) add ghost rows above and below */
    int total_rows = my_rows + 2; // two ghost rows (above and below)

    int *my_A_plus_ghosts = calloc(total_rows * N, sizeof(int));
    int *upper_ghost = my_A_plus_ghosts;                     // row -1
    int *local_data  = my_A_plus_ghosts + N;                 // rows [0 ... my_rows-1]
    int *lower_ghost = my_A_plus_ghosts + (my_rows + 1) * N; // row + my_rows

    /* TIMING START **********************************************/
    double start, end, local_elaps, global_elaps;
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    /* TIMING START **********************************************/
    
    /* MASTER: DATA DISTRIBUTION */
    MPI_Scatterv(
        A, sendcounts, senddispls, MPI_INT,
        local_data, sendcounts[my_rank], MPI_INT,
        0, MPI_COMM_WORLD
    );

    /* 2) Halo exchange rows */
    // if i have no rows (my_rows==0), no data to send else, the 
    // offset of the last row to send is the last row of my local data
    int send_down_offset = (my_rows > 0 ? (my_rows - 1) * N : 0);

    if (exchange_mode == 0) {
        // SSend / Recv version ********************************************************************
        if (my_rank > 0 && my_rows > 0) {
            MPI_Ssend(local_data, N, MPI_INT, up, UP_TAG, MPI_COMM_WORLD);
            MPI_Recv(upper_ghost, N, MPI_INT, up, DOWN_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    
        if (my_rank < num_proc - 1 && my_rows > 0) {
            if (sendcounts[down] > 0) {
                MPI_Recv(lower_ghost, N, MPI_INT, down, UP_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Ssend(local_data + send_down_offset, N, MPI_INT, down, DOWN_TAG, MPI_COMM_WORLD);
            }
        }
    } else if (exchange_mode == 1) { 
        // Isend / Irecv version ******************************************************************** 
        MPI_Request requests[4];
        int req_count = 0;
    
        // receive first
        if (my_rank > 0 && my_rows > 0) {
            MPI_Irecv(upper_ghost, N, MPI_INT, up, DOWN_TAG, MPI_COMM_WORLD, &requests[req_count++]);
        }
        if (my_rank < num_proc - 1 && my_rows > 0 && sendcounts[down] > 0) {
            MPI_Irecv(lower_ghost, N, MPI_INT, down, UP_TAG, MPI_COMM_WORLD, &requests[req_count++]);
        }
    
        // send after
        if (my_rank > 0 && my_rows > 0) {
            MPI_Isend(local_data, N, MPI_INT, up, UP_TAG, MPI_COMM_WORLD, &requests[req_count++]);
        }
        if (my_rank < num_proc - 1 && my_rows > 0 && sendcounts[down] > 0) {
            MPI_Isend(local_data + send_down_offset, N, MPI_INT, down, DOWN_TAG, MPI_COMM_WORLD, &requests[req_count++]);
        }
    
        // wait for all to complete
        if (req_count > 0) {
            MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);
        }
    } else if (exchange_mode == 2) {
        // Sendrecv version ********************************************************************
        // Exchange from UP: send my first row, receive the row above me
        MPI_Sendrecv(local_data, N, MPI_INT, up, UP_TAG,
                    upper_ghost, N, MPI_INT, up, DOWN_TAG,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        // Exchange from DOWN: send my last row, receive the row below me
        MPI_Sendrecv(local_data + send_down_offset, N, MPI_INT, down, DOWN_TAG,
                    lower_ghost, N, MPI_INT, down, UP_TAG,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } 
    


    /* 3. Parallel processing (each process on its local domain) */
    for (int i = 0; i < my_rows; i++) { // handles only valid rows
        for (int j = 0; j < N; j++) {

            // initialize accumulators
            int sum = 0; // sum of the neighborhood values
            int count = 0;    // number of elements, for the mean

            /* 3.1 neighborhood  bounds */
            // vertical 
            int zmin = (i == 0 && my_rank == 0) ? 0 : -1; // top boundary
            int zmax = (i == my_rows - 1 && my_rank == num_proc - 1) ? 0 : 1; // bottom boundary

            // horizontal
            int wmin = (j > 0) ? -1 : 0; // left boundary
            int wmax = (j < N - 1) ? 1 : 0; // right boundary

            /* 3.2 scan the 3x3 neighborhood */
            for (int dz = zmin; dz <= zmax; dz++) { // vertical offset
                int rowIndex = i + 1 + dz; // +1 to skip upper ghost

                for (int dw = wmin; dw <= wmax; dw++) { // horizontal offset
                    int colIndex = j + dw;

                    sum += my_A_plus_ghosts[rowIndex * N + colIndex];
                    count++;
                }
            }

            /* 3.5 thresholding operation */
            my_T[i * N + j] = (local_data[i * N + j] * count > sum) ? 1 : 0;
        }
    }

    free(my_A_plus_ghosts);

    /* MASTER: DATA GATHERING */
    MPI_Gatherv(
        my_T, (my_rows * N), MPI_INT,      // send buffer
        T, sendcounts, senddispls, MPI_INT, // recv buffers
        0, MPI_COMM_WORLD
    );

    /* TIMING END **********************************************/
    end = MPI_Wtime();
    local_elaps = end - start;
    MPI_Reduce(&local_elaps, &global_elaps, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    /* TIMING END **********************************************/

    /* Matrix print if not in quiet mode  */
    if (my_rank == 0) {
        if (benchmark) {
            printf("%d,%d,%f\n", num_proc, N, global_elaps);
        } else if (!quiet) {
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N ; j++) {
                    printf("%d ", T[i * N + j]);
                }
                printf("\n");
            }
        }
        
        free(A);
        free(T);
    }

    if (my_rows > 0) {
        free(my_T);
    }

    free(sendcounts);
    free(senddispls);
    
    MPI_Finalize();

    return EXIT_SUCCESS;
}