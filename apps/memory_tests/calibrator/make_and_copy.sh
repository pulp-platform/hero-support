#!/bin/bash

# Name of the program
APP=calibrator

# Make
vivado-2015.1 make all

# Copy
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
# Run script
scp run_calibrator.sh ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/${APP}/.
