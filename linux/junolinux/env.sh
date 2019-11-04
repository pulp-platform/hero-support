#!/bin/bash

# Project env setup script for Juno Linux HOWTO

# HERO target - adjust to your local setup
if [[ -z "${HERO_TARGET_HOST}" ]]; then
  export HERO_TARGET_HOST="root@arm-juno"
  export HERO_TARGET_PATH="/mnt/nfs"
fi

# Workstation connected to the Juno ADP
export JUNO_WORKSTATION_TARGET_HOST=juno@bordcomputer
export JUNO_WORKSTATION_TARGET_PATH="~/imgs"

# Kernel configuration
export KERNEL_CROSS_COMPILE=aarch64-linux-gnu-
export KERNEL_ARCH=arm64

# Linaro and OpenEmbedded configuration
export LINARO_RELEASE=16.03
export TYPE=oe

if [ ${LINARO_RELEASE} == "16.03" ]; then
  export OE_RELEASE=16.03
  export MANIFEST=latest
elif [ ${LINARO_RELEASE} == "17.04" ]; then
  export OE_RELEASE=17.01
  export MANIFEST=lsk
elif [ ${LINARO_RELEASE} == "17.10" ]; then
  export OE_RELEASE=17.01
  export MANIFEST=ack
fi

# Paths
export PYTHON_LIBS_LOCAL=~/local
export BIN_LOCAL=~/bin

# Make sure we have the cross compilers in the PATH
if [[ ":$PATH:" == *":${PWD}/gcc-linaro/aarch64-linux-gnu/bin:"* ]]; then
  echo "Your PATH is correctly set. Skipping installation."
else
  export PATH="${PWD}/gcc-linaro/aarch64-linux-gnu/bin":"${PWD}/gcc-linaro/arm-linux-gnueabihf/bin":${PATH}
fi

# Make sure we have the python libs in the PYTHONPATH
if [[ ":$PYTHONPATH:" == *":${PYTHON_LIBS_LOCAL}/lib64/python:"* ]] || \
   [[ ":$PYTHONPATH:" == *":${PYTHON_LIBS_LOCAL}/lib/python:"* ]]; then
  echo "Your PYTHONPATH is correctly set. Skipping installation."
else
  export PYTHONPATH="${PYTHON_LIBS_LOCAL}/lib64/python":${PYTHON_LIBS_LOCAL}/lib/python:${PYTHONPATH}
fi

# Make sure we have local binaries in PATH
if [[ ":$PATH:" == *":${BIN_LOCAL}:"* ]]; then
  echo "Your PATH is correctly set. Skipping installation."
else
  export PATH=${BIN_LOCAL}:${PATH}
fi
if [[ ":$PATH:" == *":${BIN_LOCAL}/bitbake/bin:"* ]]; then
  echo "Your PATH is correctly set. Skipping installation."
else
  if [ -d "${BIN_LOCAL}/bitbake/bin" ]; then
    export PATH=${BIN_LOCAL}/bitbake/bin:${PATH}
  fi
fi

# Build configuration
HERO_PARALLEL_BUILD=
if [ -z "${HERO_MAX_PARALLEL_BUILD_JOBS}" ]; then
  HERO_PARALLEL_BUILD=-j`nproc`
else
  HERO_PARALLEL_BUILD=-j${HERO_MAX_PARALLEL_BUILD_JOBS}
fi

# OpenEmbedded configuration
if [ -z "${OE_HOME}" ]; then
  OE_HOME=`realpath .`
  export OE_HOME="${OE_HOME}/oe_${OE_RELEASE}/openembedded"
fi

# Avoid repo command line prompt for name and email address
if [ -z "`git config --get user.name`" ] || [ -z "`git config --get user.email`" ]; then

  echo "Setting global GIT user name and email now."

  git config --global user.name ${USER}
  git config --global user.email ${USER}@pulp-platform.org
fi

# Set number of cores for Linaro build
export PARALLELISM=`echo ${HERO_PARALLEL_BUILD} | cut -b 3-`

# Bitbake can easily fully saturate the system - leave at least one thread.
if [ ${PARALLELISM} == `nproc` ]; then
  export BB_PARALLELISM=`expr ${PARALLELISM} - 1`
else
  export BB_PARALLELISM=${PARALLELISM}
fi
export BB_PARALLEL_BUILD="-j${BB_PARALLELISM}"
