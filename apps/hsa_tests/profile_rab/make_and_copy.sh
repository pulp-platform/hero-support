#!/bin/bash

# Name of the program
APP=profile_rab

# Make
vivado-2015.1 make all

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}/${APP}"

# Copy
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
