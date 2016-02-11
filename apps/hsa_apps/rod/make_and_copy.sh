#!/bin/bash

# Name of the program
APP=rod

# Make
vivado-2015.1 make all

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}/${APP}"

# Copy
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
# Sample files
scp -r samples ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
# Host sources for GDB
scp src/*.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
scp src/*.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
