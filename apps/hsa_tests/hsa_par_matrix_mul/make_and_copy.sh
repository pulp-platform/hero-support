#!/bin/bash

# Name of the program
NAME=hsa_par_matrix_mul

# Make
vivado-2015.1 make all

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Accelerator binary
scp ${PULP_BIN_PATH}/hsa_tests/*.bin ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.