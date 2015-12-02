#!/bin/bash

# Copy
# Scripts
scp *.sh ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
# Sample files
scp -r example ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/random_forest/.
