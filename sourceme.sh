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
export PLATFORM="4"

# Platform dependent variables
if [ "${PLATFORM}" -eq "4" ]; then
    echo "Configuring for JUNO platform"

    if [ "$(hostname | cut -d '.' -f 1)" = "fliana" ]; then
        readonly PREFIX=/scratch/vogelpi
    else
        readonly PREFIX=/usr/scratch/fliana/vogelpi
    fi

    # Linaro variables
    export LINARO_RELEASE=16.03
    export OE_RELEASE=15.09
    export MANIFEST=latest
    export TYPE=oe

    # system workspace directories - on Ubuntu machine
    export WORKSPACE_DIR=${PREFIX}
    export LINARO_DIR=${PREFIX}/linaro-${LINARO_RELEASE}
    export OE_DIR=${PREFIX}/oe-${OE_RELEASE}

    # directory containing the kernel sources
    export KERNEL_DIR=${LINARO_DIR}/workspace/linux/out/juno-oe

    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="juno@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/juno/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/juno/share/drivers"

    # path to ARM libraries: external (opencv, ffmpeg), libc, shared libs in filesystem
    export ARM_LIB_EXT_DIR=${PREFIX}/libs-juno/lib
    export ARM_LIBC_DIR=${OE_DIR}/openembedded/build/tmp-glibc/sysroots/lib32-genericarmv8/lib
    export ARM_LIB_FS_DIR=${OE_DIR}/openembedded/build/tmp-glibc/sysroots/lib32-genericarmv8/usr
    export ARM_SYSROOT_DIR=${OE_DIR}/openembedded/build/tmp-glibc/sysroots/lib32-genericarmv8

    # number of cores on Linux host
    export N_CORES_COMPILE=4

    # ARCH and CROSS_COMPILE variables
    export KERNEL_ARCH="arm64"
    export KERNEL_CROSS_COMPILE="aarch64-linux-gnu-"

    export ARCH="arm"
    export CROSS_COMPILE="arm-linux-gnueabihf-"
    export TUPLE="arm-linux-gnueabihf"

    # GCC version 4.9, 5.2
    GCC_VERSION="4.9"

    # Set up PATH variable
    export PATH=${PREFIX}/cross/linaro_gcc_${GCC_VERSION}/aarch64-linux-gnu/bin:${PREFIX}/cross/linaro_gcc_${GCC_VERSION}/arm-linux-gnueabihf/bin:$PATH

	# directory containing PULP header files - on CentOS machine - needs to be accessible also by Ubuntu machine
	export PULP_INC_DIR1=/home/vogelpi/riseten-scratch/juno/pulp_pipeline/pkg/sdk/dev/install/include/archi/bigpulp
	export PULP_INC_DIR2=/home/vogelpi/riseten-scratch/juno/pulp_pipeline/pkg/sdk/dev/install/include
else
    echo "Configuring for ZYNQ platform"

    if [ "$(hostname | cut -d '.' -f 1)" = "riseten" ]; then
        readonly PREFIX=/scratch/vogelpi
    else
        readonly PREFIX=/usr/scratch/riseten/vogelpi
    fi

    # system workspace directory - on CentOS machine
    if   [ "${PLATFORM}" -eq "1" ]; then
    	export WORKSPACE_DIR=${PREFIX}/zedboard
    elif [ "${PLATFORM}" -eq "2" ]; then
        export WORKSPACE_DIR=${PREFIX}/zc706
    else
    	export WORKSPACE_DIR=${PREFIX}/mini-itx
    fi

    # directory containing the kernel sources
    export KERNEL_DIR=${WORKSPACE_DIR}/workspace/linux-xlnx

    # machine to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_MACHINE="vogelpi@bordcomputer"

    # base path to which the make_and_copy.sh scripts transfer the compiled stuff
    export SCP_TARGET_PATH="~/pulp_on_fpga/share/apps"
    export SCP_TARGET_PATH_DRIVERS="~/pulp_on_fpga/share/drivers"

    # path to ARM libraries: external (opencv, ffmpeg), libc, shared libs in filesystem
    export ARM_LIB_EXT_DIR=${PREFIX}/libs-zynq/lib
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
	export PULP_INC_DIR1=${WORKSPACE_DIR}/pulp_pipeline/pkg/sdk/dev/install/include/archi/bigpulp
	export PULP_INC_DIR2=${WORKSPACE_DIR}/pulp_pipeline/pkg/sdk/dev/install/include
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
