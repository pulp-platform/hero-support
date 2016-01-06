#!/bin/bash

# Name of the program
APP=rab_miss_handling

# Make
vivado-2015.1 make all

# Copy
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}/programs/${APP}"
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
# Accelerator binary
scp ${PULP_SW_PATH}/hsa_tests/${APP}/*.bin ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.