#!/bin/bash

# Name of the program
NAME=CRForest-Detector

# Make
vivado-2015.1 make all

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
# Scripts
#scp *.sh ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
# Sample files
#scp -r example ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
