#!/bin/bash

# Setup script for BusyBox

# Make sure required variables are set
if [ -z "${KERNEL_ARCH}" ] || [ -z "${KERNEL_CROSS_COMPILE}" ] ; then
  echo "ERROR: Missing required environment variables KERNEL_ARCH and/or KERNEL_CROSS_COMPILE, aborting now."
  exit 1;
fi

# Info
echo "-----------------------------------------"
echo "-  Executing BusyBox setup script   -"
echo "-----------------------------------------"

# Setup BusyBox
echo "Downloading BusyBox sources"
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} busybox-source

# Overwrite BusyBox config
cd output/build/busybox-*/
cp ../../../busybox-config .config
cd ../../../
