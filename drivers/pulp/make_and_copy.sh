#!/bin/bash

if [ "${PLATFORM}" -eq "4" ]; then # JUNO
  # Make
  make

  # .read
  ${KERNEL_CROSS_COMPILE}objdump -DS pulp.ko > pulp.read
else # ZYNQ
  # Make
  vivado-2015.1 make

  # .read
  vivado-2015.1 ${KERNEL_CROSS_COMPILE}-objdump -DS pulp.ko > pulp.read
fi

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH_DRIVERS}"

# Copy
scp *.ko ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH_DRIVERS}/.
