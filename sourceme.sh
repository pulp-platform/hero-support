#!/bin/bash

# Script to set shell variables required to cross compile code for the ARM host
#
# Adjust to your local setup
#
# Call using soure soureme.sh

# Xilinx Vivado version for cross-compilation toolchain
export VIVADO_VERSION="vivado-2015.1"

# system workspace directory
export WORKSPACE_DIR=/scratch/vogelpi/mini-itx/workspace

# directory containing the kernel sources
export KERNEL_DIR=${WORKSPACE_DIR}/linux-xlnx

# top directory containing custom ARM libraries
export ARM_LIB_DIR1=/home/vogelpi/pulp_on_fpga/software/arm/lib

# directory containing ARM header files
export ARM_INC_DIR1=${ARM_LIB_DIR1}/inc

# directory containing PULP header files
export PULP_INC_DIR1=/home/vogelpi/pulp_on_fpga/software/pulp/pulp_pipeline/pkg/sdk/dev/install/include/archi/pulp4
export PULP_INC_DIR2=/home/vogelpi/pulp_on_fpga/software/pulp/pulp_pipeline/pkg/sdk/dev/install/include/pulp4

# PULP HSA home directory
export PULP_HSA_HOME=/home/vogelpi/pulp_on_fpga/software/hsa_sw

# machine to which the make_and_copy.sh scripts transfer the compiled stuff
export SCP_TARGET_MACHINE="vogelpi@bordcomputer"

# base path to which the make_and_copy.sh scripts transfer the compiled stuff
export SCP_TARGET_PATH="~/pulp_on_fpga/share/programs"
export SCP_TARGET_PATH_DRIVERS="~/pulp_on_fpga/share/drivers"

# create lib/lib folder if not existing
if [ ! -d ${ARM_LIB_DIR1}/lib ]
then
    mkdir ${ARM_LIB_DIR1}/lib
fi    

# PULP binary path, i.e., where the compiled accelerator binaries can be found
export PULP_SW_PATH=/home/vogelpi/pulp_on_fpga/software/pulp

# path to external ARM libraries (on scratch)
export ARM_LIB_EXT_DIR=/scratch/vogelpi/libs-zynq/lib

# number of cores on Linux host
export N_CORES_COMPILE=8