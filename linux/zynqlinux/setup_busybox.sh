#!/bin/bash

# Setup script for BusyBox

# Default variables
if [[ -z "KERNEL_ARCH" ]]; then
  export KERNEL_ARCH="arm"
fi
if [[ -z "KERNEL_CROSS_COMPILE" ]]; then
  export KERNEL_CROSS_COMPILE="arm-xilinx-linux-gnueabi-"
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
