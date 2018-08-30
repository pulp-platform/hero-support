#!/bin/bash

# Compile script for Buildroot

# Make sure required variables are set
if [ -z "${KERNEL_ARCH}" ] || [ -z "${KERNEL_CROSS_COMPILE}" ] ; then
  echo "ERROR: Missing required environment variables KERNEL_ARCH and/or KERNEL_CROSS_COMPILE, aborting now."
  exit 1;
fi

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
