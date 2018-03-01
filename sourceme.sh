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
#  TE0808:   5
export PLATFORM="2"

# Platform dependent variables

case ${PLATFORM} in
    1)  export BOARD="zedboard";;
    2)  export BOARD="zc706";;
    3)  export BOARD="mini-itx";;
    4)  export BOARD="juno";;
    5)  export BOARD="te0808";;
    *)
        echo "[EE] Failed to determine BOARD from PLATFORM!"
        exit 1
        ;;
esac

if [ "${PLATFORM}" -eq "4" ]; then
    echo "Configuring for JUNO platform"

    # system workspace directory - on Ubuntu machine
    export WORKSPACE_DIR=/scratch/vogelpi

    # Linaro variables
    export LINARO_RELEASE=16.03
    export OE_RELEASE=16.03
    export MANIFEST=latest
    export TYPE=oe

    # directory containing the kernel sources
    export KERNEL_DIR=${WORKSPACE_DIR}/linaro-${LINARO_RELEASE}/workspace/linux/out/juno-oe

    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="juno@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/juno/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/juno/share/drivers"

    # path to ARM libraries: external (opencv, ffmpeg), libc, shared libs in filesystem
    export ARM_LIB_EXT_DIR=${WORKSPACE_DIR}/libs-juno/lib
    export ARM_LIBC_DIR=${WORKSPACE_DIR}/oe-${OE_RELEASE}/openembedded/build/tmp-glibc/sysroots/lib32-genericarmv8/lib
    export ARM_LIB_FS_DIR=${WORKSPACE_DIR}/oe-${OE_RELEASE}/openembedded/build/tmp-glibc/sysroots/lib32-genericarmv8/usr
    export ARM_SYSROOT_DIR=${WORKSPACE_DIR}/oe-${OE_RELEASE}/openembedded/build/tmp-glibc/sysroots/lib32-genericarmv8

    # number of cores on Linux host
    export N_CORES_COMPILE=4

    # ARCH and CROSS_COMPILE variables
    export KERNEL_ARCH="arm64"
    export KERNEL_CROSS_COMPILE="aarch64-linux-gnu-"

    export ARCH="arm"
    export CROSS_COMPILE="arm-linux-gnueabihf-"
    export TUPLE="arm-linux-gnueabihf"

    # GCC version 4.9, 5.2
    GCC_VERSION="5.2" 

    # Set up PATH variable
    export PATH=${WORKSPACE_DIR}/cross/linaro_gcc_${GCC_VERSION}/aarch64-linux-gnu/bin/:${WORKSPACE_DIR}/cross/linaro_gcc_${GCC_VERSION}/arm-linux-gnueabihf/bin/:$PATH

	# directory containing PULP header files - on CentOS machine - needs to be accessible also by Ubuntu machine
	export PULP_INC_DIR1=/home/vogelpi/riseten-scratch/juno/pulp_pipeline/pkg/sdk/dev/install/include/archi/bigpulp
	export PULP_INC_DIR2=/home/vogelpi/riseten-scratch/juno/pulp_pipeline/pkg/sdk/dev/install/include
elif [ "${PLATFORM}" -eq "5" ]; then
    echo "Configuring for TE0808 ZYNQ MPSOC platform"

    # system workspace directory - on CentOS machine
    export WORKSPACE_DIR=/scratch/vogelpi/te0808

    # directory containing the kernel sources
    #export KERNEL_DIR=${WORKSPACE_DIR}/petalinux/TEBF0808/os/wimmis02/build/linux/kernel/link-to-kernel-build
    #export KERNEL_DIR=/usr/scratch/larain4/vogelpi/te0808/petalinux/TEBF0808/os/wimmis02/components/ext_sources/linux-xlnx
    #export KERNEL_DIR=/usr/scratch/larain4/vogelpi/te0808/petalinux/TEBF0808/os/wimmis02/build/tmp/work/plnx_aarch64-xilinx-linux/linux-xlnx/4.9-xilinx-v2017.2+git999-r0/linux-xlnx-4.9-xilinx-v2017.2+git999
    export KERNEL_DIR=/scratch/vogelpi/te0808/petalinux/TEBF0808/os/wimmis02/build/tmp/work/plnx_aarch64-xilinx-linux/linux-xlnx/4.9-xilinx-v2017.2+git999-r0/linux-xlnx-4.9-xilinx-v2017.2+git999


    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="vogelpi@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/zynqmp/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/zynqmp/share/drivers"

    # path to ARM libraries: external (opencv, ffmpeg), libc, shared libs in filesystem
    export ARM_LIB_EXT_DIR=${WORKSPACE_DIR}/../libs-zynqmp/lib
    #export ARM_LIBC_DIR=${WORKSPACE_DIR}/petalinux/TEBF0808/os/wimmis02/build/linux/rootfs/targetroot/lib
    #export ARM_LIB_FS_DIR=${WORKSPACE_DIR}/petalinux/TEBF0808/os/wimmis02/build/linux/rootfs/targetroot/usr
    export ARM_LIBC_DIR=/usr/scratch/larain4/vogelpi/te0808/petalinux/TEBF0808/os/wimmis02/build/tmp/sysroots/lib32-plnx_aarch64/lib
    export ARM_LIB_FS_DIR=/usr/scratch/larain4/vogelpi/te0808/petalinux/TEBF0808/os/wimmis02/build/tmp/sysroots/lib32-plnx_aarch64/usr

    # number of cores on Linux host
    export N_CORES_COMPILE=8

    # ARCH and CROSS_COMPILE variables
    export KERNEL_ARCH="arm64"
    export KERNEL_CROSS_COMPILE="aarch64-linux-gnu-"

    export ARCH="arm"
    export CROSS_COMPILE="arm-xilinx-linux-gnueabi-"
    export TUPLE="arm-xilinx-linux-gnueabi"

    # Set up PATH variable
    #${VIVADO_VERSION} bash

    # directory containing PULP header files - on CentOS machine
	export PULP_INC_DIR1=${WORKSPACE_DIR}/pulp-sdk/pkg/sdk/dev/install/include/archi/chips/bigpulp
	export PULP_INC_DIR2=${WORKSPACE_DIR}/pulp-sdk/pkg/sdk/dev/install/include
else 
    echo "Configuring for ZYNQ platform"

    # system workspace directory - on CentOS machine
    if   [ "${PLATFORM}" -eq "1" ]; then
    	export WORKSPACE_DIR=/scratch/vogelpi/zedboard
        export PULP_CURRENT_CONFIG="system=bigpulp:platform=hsa:nb_cluster=1"
    elif [ "${PLATFORM}" -eq "2" ]; then
        export WORKSPACE_DIR=/scratch/vogelpi/zc706
        export PULP_CURRENT_CONFIG="system=bigpulp-z-7045:platform=hsa:nb_cluster=1"
    else
    	export WORKSPACE_DIR=/scratch/vogelpi/mini-itx
    fi

    # directory containing the kernel sources
    export KERNEL_DIR=${WORKSPACE_DIR}/workspace/linux-xlnx

    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="vogelpi@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/pulp_on_fpga/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/pulp_on_fpga/share/drivers"

    # path to ARM libraries: external (opencv, ffmpeg), libc, shared libs in filesystem
    export ARM_LIB_EXT_DIR=${WORKSPACE_DIR}/../libs-zynq/lib
    export ARM_LIBC_DIR=/usr/pack/vivado-2015.1-kgf/SDK/2015.1/gnu/arm/lin/arm-xilinx-linux-gnueabi/libc/lib
    export ARM_LIB_FS_DIR=${WORKSPACE_DIR}/workspace/buildroot/output/target/usr

    # number of cores on Linux host
    export N_CORES_COMPILE=8

    # ARCH and CROSS_COMPILE variables
    export KERNEL_ARCH="arm"
    export KERNEL_CROSS_COMPILE="arm-xilinx-linux-gnueabi-"

    export ARCH="arm"
    export CROSS_COMPILE="arm-xilinx-linux-gnueabi-"
    export TUPLE="arm-xilinx-linux-gnueabi"

    # Set up PATH variable
    #${VIVADO_VERSION} bash

    # directory containing PULP header files - on CentOS machine
    export PULP_INC_DIR1=${WORKSPACE_DIR}/pulp-sdk/pkg/sdk/dev/install/include/archi/chips/bigpulp
    export PULP_INC_DIR2=${WORKSPACE_DIR}/pulp-sdk/pkg/sdk/dev/install/include
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

# PULP HSA home directory
export PULP_HSA_HOME=/home/vogelpi/pulp_on_fpga/software/hsa_apps
