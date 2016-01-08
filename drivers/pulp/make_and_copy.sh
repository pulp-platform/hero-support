#!/bin/bash

# Make
vivado-2015.1 make

# .read
vivado-2015.1 arm-xilinx-linux-gnueabi-objdump -DS pulp.ko > pulp.read

# Copy
scp *.ko ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH_DRIVERS}/.
