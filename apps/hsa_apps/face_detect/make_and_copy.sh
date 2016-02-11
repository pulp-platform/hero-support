#!/bin/bash

# Name of the program
APP=face_detect

# Make
vivado-2015.1 make all

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}/programs/${APP}"

# Copy
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
# Scripts
#scp *.sh ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
# Sample files
#scp -r example ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
