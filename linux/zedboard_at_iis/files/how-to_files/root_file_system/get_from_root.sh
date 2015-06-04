#!/bin/bash

# Copy back root file system from other machine
echo "Copying root file system from ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/"
scp ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/rootfs.cpio.gz .
