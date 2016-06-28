#!/bin/bash

# Script to set shell variables required to cross compile code for the ARM host
#
# Adjust to your local setup
#
# Call using soure soureme.sh

# Xilinx Vivado version for cross-compilation toolchain
export VIVADO_VERSION="vivado-2015.1"

# Select platform:
#  ZEDBOARD: 1
#  ZC706:    2
#  MINI_ITX: 3
#  JUNO:     4
export PLATFORM="3"

# Platform dependent variables
if [ "${PLATFORM}" -eq "4" ]; then
    echo "Configuring for JUNO platform"

    export LINARO_RELEASE=16.03
    export OE_RELEASE=15.09

    # system workspace directory
    #export WORKSPACE_DIR=/usr/scratch/fliana/vogelpi/linaro-${LINARO_RELEASE}
    export WORKSPACE_DIR=/scratch/vogelpi/linaro-${LINARO_RELEASE}

    # directory containing the kernel sources
    export KERNEL_DIR=${WORKSPACE_DIR}/workspace/linux/out/juno-oe

    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="juno@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/juno/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/juno/share/drivers"

    # path to external ARM libraries (on scratch)
    #export ARM_LIB_EXT_DIR=/usr/scratch/fliana/vogelpi/libs-juno/lib
    export ARM_LIB_EXT_DIR=/scratch/vogelpi/libs-juno/lib

    # number of cores on Linux host
    export N_CORES_COMPILE=4

    # ARCH and CROSS_COMPILE variables
    export KERNEL_ARCH="arm64"
    export KERNEL_CROSS_COMPILE="aarch64-linux-gnu-"

    export ARCH="arm"
    export CROSS_COMPILE="arm-linux-gnueabihf-"

    # GCC version 4.9, 5.2
    GCC_VERSION="4.9" 

    # Set up PATH variable
    #export PATH=/usr/scratch/fliana/vogelpi/cross/linaro_gcc_${GCC_VERSION}/aarch64-linux-gnu/bin/:/usr/scratch/fliana/vogelpi/cross/linaro_gcc_${GCC_VERSION}/arm-linux-gnueabihf/bin/:$PATH
    export PATH=/scratch/vogelpi/cross/linaro_gcc_${GCC_VERSION}/aarch64-linux-gnu/bin/:/scratch/vogelpi/cross/linaro_gcc_${GCC_VERSION}/arm-linux-gnueabihf/bin/:$PATH
else 
    echo "Configuring for ZYNQ platform"

    # system workspace directory
    export WORKSPACE_DIR=/scratch/vogelpi/mini-itx

    # directory containing the kernel sources
    export KERNEL_DIR=${WORKSPACE_DIR}/workspace/linux-xlnx

    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="vogelpi@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/pulp_on_fpga/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/pulp_on_fpga/share/drivers"

    # path to external ARM libraries (on scratch)
    export ARM_LIB_EXT_DIR=/scratch/vogelpi/libs-zynq/lib

    # number of cores on Linux host
    export N_CORES_COMPILE=8

    # ARCH and CROSS_COMPILE variables
    export KERNEL_ARCH="arm"
    export KERNEL_CROSS_COMPILE="arm-xilinx-linux-gnueabi-"

    export ARCH="arm"
    export CROSS_COMPILE="arm-xilinx-linux-gnueabi-"

    # Set up PATH variable
    #${VIVADO_VERSION} bash
fi

# top directory containing custom ARM libraries
export ARM_LIB_DIR1=/home/vogelpi/pulp_on_fpga/software/hsa_support/lib

# create lib/lib folder if not existing
if [ ! -d ${ARM_LIB_DIR1}/lib ]
then
    mkdir ${ARM_LIB_DIR1}/lib
fi    

# directory containing ARM header files
export ARM_INC_DIR1=${ARM_LIB_DIR1}/inc

# directory containing PULP header files
export PULP_INC_DIR1=/home/vogelpi/riseten-scratch/juno/pulp_pipeline/pkg/sdk/dev/install/include/archi/bigpulp
export PULP_INC_DIR2=/home/vogelpi/riseten-scratch/juno/pulp_pipeline/pkg/sdk/dev/install/include

# PULP HSA home directory
export PULP_HSA_HOME=/home/vogelpi/pulp_on_fpga/software/hsa_apps

# PULP binary path, i.e., where the compiled accelerator binaries can be found
#export PULP_SW_PATH=/home/vogelpi/pulp_on_fpga/software/pulp

