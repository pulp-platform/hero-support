#!/bin/bash

# Make sure required variables are set
if [ -z "${KERNEL_ARCH}" ] || [ -z "${KERNEL_CROSS_COMPILE}" ] || [ -z "${KERNEL_OUT}" ] ; then
  echo "ERROR: Missing required environment variables KERNEL_ARCH, KERNEL_CROSS_COMPILE and/or KERNEL_OUT, aborting now."
  exit 1;
fi

# Build kernel and device tree
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} ${HERO_PARALLEL_BUILD} -Image O=${KERNEL_OUT}
make ARCH=${KERNEL_ARCH} CROSS_COMPILE=${KERNEL_CROSS_COMPILE} ${HERO_PARALLEL_BUILD} -Image O=${KERNEL_OUT} dtbs
