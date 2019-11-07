#!/bin/bash

# Project build script for Juno Linux HOWTO

# Make sure everyting is configured and that we have the correct environment variables
source env.sh

# Info
echo "-----------------------------------------"
echo "-    Executing project build script     -"
echo "-----------------------------------------"

# Linaro Linux kernel
cd linaro_${LINARO_RELEASE}
./build_linaro_kernel.sh

if [ $? -ne 0 ]; then
  echo "ERROR: build_linaro_kernel.sh failed, aborting now."
  exit 1
fi

cd ..

if [ ${OE_RELEASE} == "16.03" ]; then

  # OpenEmbedded root filesystem
  cd oe_${OE_RELEASE}
  source build_oe.sh

  if [ $? -ne 0 ]; then
    echo "ERROR: build_oe.sh failed, aborting now."
    exit 1
  fi

  cd ..
fi
