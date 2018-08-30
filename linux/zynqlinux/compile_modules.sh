#!/bin/bash

# Compile script for the loadable kernel modules

# Make sure required variables are set
if [ -z "${KERNEL_ARCH}" ] || [ -z "${KERNEL_CROSS_COMPILE}" ] ; then
  echo "ERROR: Missing required environment variables KERNEL_ARCH and/or KERNEL_CROSS_COMPILE, aborting now."
  exit 1;
fi

# Info
echo "-----------------------------------------"
echo "-    Executing modules compile script   -"
echo "-----------------------------------------"

# Compile the Linux kernel modules
echo "Comiling kernel modules"
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} modules
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} modules_install INSTALL_MOD_PATH=lib_modules

# Copy
echo "Copying kernel modules to ../root_file_system/custom_files/"
cp -r lib_modules/* ../root_filesystem/custom_files/.
