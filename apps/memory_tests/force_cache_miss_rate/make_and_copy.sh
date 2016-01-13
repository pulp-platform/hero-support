#!/bin/bash

# Name of the program
NAME=force_cache_miss_rate

# Make
${VIVADO_VERSION} make all

# Copy
# Host executable
scp ${NAME} ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${NAME}
# Host sources for GDB
scp *.c ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${NAME}
scp *.h ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/${NAME}
# Accelerator binary
#scp *.bin ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/programs/.
