#!/bin/bash

# Name of the app
NAME=standalone

if [ "${PLATFORM}" -eq "4" ]; then # JUNO
  # Make
  make all
else # ZYNQ
  # Make
  ${VIVADO_VERSION} make ll
fi

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}"

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.
