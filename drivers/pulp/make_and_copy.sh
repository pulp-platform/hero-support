#!/bin/bash

# Make
vivado-2015.1 make

# Copy
scp *.ko ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/drivers/.
