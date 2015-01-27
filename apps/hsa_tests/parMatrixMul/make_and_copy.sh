#!/bin/bash

# Name of the program
NAME=parMatrixMul

# Make
vivado-2014.1 make all

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Accelerator binary
scp *.bin ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.