#!/bin/bash

# Compile script for the loadable kernel modules

# Default variables
if [[ -z "KERNEL_ARCH" ]]; then
  export KERNEL_ARCH="arm"
fi
if [[ -z "KERNEL_CROSS_COMPILE" ]]; then
  export KERNEL_CROSS_COMPILE="arm-xilinx-linux-gnueabi-"
fi

# Info
echo "-----------------------------------------"
echo "-    Executing modules compile script   -"
echo "-----------------------------------------"

# Compile the Linux kernel modules
echo "Comiling the Linux kernel"
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} modules
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} modules_install INSTALL_MOD_PATH=lib_modules

# Copy
echo "Copying kernel modules to ../root_file_system/custom_files/"
cp -r lib_modules/* ../root_filesystem/custom_files/.
