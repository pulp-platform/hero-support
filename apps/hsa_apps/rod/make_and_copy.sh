#!/bin/bash

# Name of the program
APP=rod

# Make
vivado-2015.1 make all

# Copy
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
# Sample files
scp -r samples ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
# Host sources for GDB
scp src/*.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
scp src/*.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
# Accelerator binary
#scp ${PULP_SW_PATH}/hsa_apps/${APP}/${APP}.bin ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.