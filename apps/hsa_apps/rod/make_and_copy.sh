#!/bin/bash

# Name of the program
NAME=rod

# Make
vivado-2014.1 make all

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Sample files
scp -r samples ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Host sources for GDB
scp src/*.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
scp src/*.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
# Accelerator binary
scp *.bin ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.