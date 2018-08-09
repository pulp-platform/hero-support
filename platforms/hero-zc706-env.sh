#! /bin/bash
# Copyright (C) 2018 ETH Zurich and University of Bologna
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# HERO on ZC706 Envoronment
#

# Xilinx Vivado version for cross-compilation toolchain
# export VIVADO_VERSION="vivado-2015.1"
export PLATFORM="2"
export BOARD="zc706"

if [[ -z "${HERO_SDK_DIR}" ]]; then
	export HERO_SDK_DIR=`realpath .`
fi

# PULP Side SDK Configurations
export PULP_INC_DIR1=`realpath pulp-sdk/pkg/sdk/dev/install/include/archi/chips/bigpulp`
export PULP_INC_DIR2=`realpath pulp-sdk/pkg/sdk/dev/install/include`

# HOST Side Configurations
export ARCH="arm"
export CROSS_COMPILE="arm-linux-gnueabihf-"
export HERO_LINUX_KERNEL_DIR=`realpath linux/zynqlinux/linux-xlnx`

# machine to which the make_and_copy.sh scripts transfer the compiled stuff
# export SCP_TARGET_MACHINE="vogelpi@bordcomputer"

# base path to which the make_and_copy.sh scripts transfer the compiled stuff
# export SCP_TARGET_PATH="~/pulp_on_fpga/share/apps"
# export SCP_TARGET_PATH_DRIVERS="~/pulp_on_fpga/share/drivers"

# path to ARM libraries: external (opencv, ffmpeg), libc, shared libs in filesystem
# export ARM_LIB_EXT_DIR=${HERO_SDK}/../libs-zynq/lib
# export ARM_LIBC_DIR=/usr/pack/vivado-2015.1-kgf/SDK/2015.1/gnu/arm/lin/arm-xilinx-linux-gnueabi/libc/lib
# export ARM_LIB_FS_DIR=${HERO_SDK}/workspace/buildroot/output/target/usr

# number of cores on Linux host
# export N_CORES_COMPILE=12

# ARCH and CROSS_COMPILE variables
# export KERNEL_ARCH="arm"
# export KERNEL_CROSS_COMPILE="arm-linux-gnueabihf-"
# export TUPLE="arm-linux-gnueabihf"

# top directory containing custom ARM libraries
# export ARM_LIB_DIR1=`realpath .`

# create lib/lib folder if not existing
# if [ ! -d ${ARM_LIB_DIR1}/lib ]
# then
    # mkdir ${ARM_LIB_DIR1}/lib
# fi    

# directory containing ARM header files
# export ARM_INC_DIR1=${ARM_LIB_DIR1}/inc

# # PULP HSA home directory
# export PULP_HSA_HOME=/home/vogelpi/pulp_on_fpga/software/hsa_apps

# That's all folks!!
