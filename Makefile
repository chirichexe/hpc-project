CC_MPI    = mpicc
CC_SERIAL = gcc
CFLAGS    = -Wall -Wextra -O0 -march=native
OMP_FLAGS = -fopenmp

# dirs
SRC_DIR = src
BIN_DIR = bin

# executable files
TARGETS = $(BIN_DIR)/binarize_mpi \
          $(BIN_DIR)/binarize_omp \
          $(BIN_DIR)/binarize_serial

# default: compile all
all: $(BIN_DIR) $(TARGETS)

# only MPI
$(BIN_DIR)/binarize_mpi: $(SRC_DIR)/bynarize_MPI.c
	$(CC_MPI) $(CFLAGS) $< -o $@

# only OpenMP
$(BIN_DIR)/binarize_omp: $(SRC_DIR)/bynarize_OMP.c
	$(CC_SERIAL) $(CFLAGS) $(OMP_FLAGS) $< -o $@

# only pure C
$(BIN_DIR)/binarize_serial: $(SRC_DIR)/bynarize_serial.c
	$(CC_SERIAL) $(CFLAGS) $< -o $@

# cleanup
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
