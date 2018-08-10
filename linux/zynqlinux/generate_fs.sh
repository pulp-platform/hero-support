#!/bin/bash

# Compile script for Buildroot

# Default variables
if [[ -z "KERNEL_ARCH" ]]; then
  export KERNEL_ARCH="arm"
fi
if [[ -z "KERNEL_CROSS_COMPILE" ]]; then
  export KERNEL_CROSS_COMPILE="arm-xilinx-linux-gnueabi-"
fi
# Call using vivado-2014.1 ./generate_fs.sh

# Info
echo "-----------------------------------------"
echo "-  Executing Buildroot compile script   -"
echo "-----------------------------------------"

# Build the root file system
echo "Generating the root file system"
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} $1

# Copy
echo "Copying the root file system image to ../root_file_system/rootfs.cpio.gz"
cp output/images/rootfs.cpio.gz ../root_filesystem/.
