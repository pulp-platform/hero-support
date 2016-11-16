#!/bin/bash

# Name of the program
APP=force_cache_miss_rate

if [ "${PLATFORM}" -eq "4" ]; then # JUNO
  # Make
  make all
else # ZYNQ
  # Make
  ${VIVADO_VERSION} make all
fi

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH}/${APP}"

# Copy
# Host executable
scp ${APP} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
# Test data
scp -r test ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${APP}/.
