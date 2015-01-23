#!/bin/bash

# Script to set shell variables required to cross compile code for the ARM host
#
# Adjust to your local setup
#
# Call using soure soureme.sh

# directory containing the kernel sources
export KERNEL_DIR=/scratch/vogelpi/mini-itx/workspace/linux-xlnx

# top directory containing custom ARM libraries
export ARM_LIB_DIR1=/home/vogelpi/pulp_on_fpga/software/arm/lib

# directory containing ARM header files
export ARM_INC_DIR1=${ARM_LIB_DIR1}/inc

# directory containing PULP header files
export PULP_INC_DIR1=/home/vogelpi/pulp_on_fpga/hardware/git/pulpemu/pulp2/sw/libs/sys_lib/inc

