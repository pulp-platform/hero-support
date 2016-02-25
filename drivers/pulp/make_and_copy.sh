#!/bin/bash

# Make
vivado-2015.1 make

# .read
vivado-2015.1 arm-xilinx-linux-gnueabi-objdump -DS pulp.ko > pulp.read

# Create folder on target
ssh ${SCP_TARGET_MACHINE} "mkdir -p ${SCP_TARGET_PATH_DRIVERS}"

# Copy
scp *.ko ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH_DRIVERS}/.
