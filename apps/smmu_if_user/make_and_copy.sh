#!/bin/bash

# Name of the app
NAME=smmu_if_user

# Make
make all

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}"

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.
# Host sources for GDB
#scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.
#scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.
