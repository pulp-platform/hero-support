#!/bin/bash

# Project build script for Zynq Linux HOWTO

# Function to replace and backup config files
take_newer() {

  # Two arguments are required
  if [ $# -ne 2 ]; then
    echo "ERROR: Require exactly 2 arguments."
    exit 1
  fi

  # Does File $2 already exist
  if [ -f $2 ]; then
    if test $2 -nt $1; then
      # Nothing needs to be done
      echo "Continuing with current, newer $2."
    else
      # Backup current file, take newer input file
      echo "Backing up current $2, replacing by $1."

      BKP_NAME_1=`echo $2 | sed "s/^\.//"`
      BKP_NAME_2=`stat --printf='%.19y\n' $2`
      BKP_NAME_2=`echo ${BKP_NAME_2} | sed "s/ /_/"`
      BKP_NAME_2=`echo ${BKP_NAME_2} | sed "s/:/-/g"`
      BKP_NAME="${BKP_NAME_1}_${BKP_NAME_2}"

      mv $2 ${BKP_NAME}
      cp $1 $2
    fi
  else
    echo "Installing $2 from $1."
    cp $1 $2
  fi
}

# Info
echo "-----------------------------------------"
echo "-    Executing project build script     -"
echo "-----------------------------------------"

# Make sure required variables are set
if [ -z "${KERNEL_ARCH}" ] || [ -z "${KERNEL_CROSS_COMPILE}" ] ; then
  echo "ERROR: Missing required environment variables KERNEL_ARCH and/or KERNEL_CROSS_COMPILE, aborting now."
  exit 1;
fi

DOWNLOAD_URL="https://pulp-platform.org/hero/files/images/zc706/"
DOWNLOAD_ARGS="--no-verbose -N -U firefox"

# Build configuration
HERO_PARALLEL_BUILD=
if [ -z "${HERO_MAX_PARALLEL_BUILD_JOBS}" ]; then
  HERO_PARALLEL_BUILD=-j`nproc`
else
  HERO_PARALLEL_BUILD=-j${HERO_MAX_PARALLEL_BUILD_JOBS}
fi


#################################################
#
# Linux kernel
#
#################################################

# Try to build kernel (might fail because of missing U-Boot mkimage)
cd linux-xlnx
{
  ./compile_kernel.sh  ${HERO_PARALLEL_BUILD}
} || {
  COMPILE_KERNEL_FAILED=1
}
cd ..

if [[ ! -z "${COMPILE_KERNEL_FAILED}" ]]; then
  echo "Linux kernel build failed as expected. I first need to build U-Boot mkimage."

  # Build U-Boot (needed for kernel build)
  cd u-boot-xlnx
  ./compile_loader.sh ${HERO_PARALLEL_BUILD}
  cd ..

  # Rebuild kernel
  cd linux-xlnx
  ./compile_kernel.sh  ${HERO_PARALLEL_BUILD}

  if [ $? -ne 0 ]; then
    echo "ERROR: compile_kernel.sh failed, aborting now."
    exit 1
  fi

  cd ..
fi

# Also build modules
cd linux-xlnx
./compile_modules.sh ${HERO_PARALLEL_BUILD}

if [ $? -ne 0 ]; then
  echo "ERROR: compile_modules.sh failed, aborting now."
  exit 1
fi

cd ..

#################################################
#
# U-Boot
#
#################################################

cd u-boot-xlnx
./compile_loader.sh ${HERO_PARALLEL_BUILD}
cd ..

#################################################
#
# Device tree blob
#
#################################################

# Check if Xilinx SDK-generated device tree sources exists - otherwise download them
cd sdk

if [ ! -d device_tree_bsp_0 ]; then
  mkdir device_tree_bsp_0
fi

cd device_tree_bsp_0

for file in pcw.dtsi pl.dtsi skeleton.dtsi system.dts system-top.dts zynq-7000.dtsi; do
  if [ ! -f ${file} ]; then
    wget ${DOWNLOAD_URL}${file} ${DOWNLOAD_ARGS}
  fi
done

cd ../../

# Generate the device tree blob
cd linux-xlnx
./generate_dtb.sh

if [ $? -ne 0 ]; then
  echo "ERROR: generate_dtb.sh failed, aborting now."
  exit 1
fi

cd ..

#################################################
#
# BOOT.bin
#
#################################################

# Check if Vivado is installed or sourced - otherwise we cannot generate BOOT.bin and must download it.
which bootgen
if [ $? -eq 0 ]; then
  # Check if Xilinx SDK-generated FSBL exists - otherwise download it
  cd sdk
  
  if [ ! -d FSBL/Debug ]; then
    mkdir -p FSBL/Debug
  fi
  
  cd FSBL/Debug
  
  if [ ! -f FSBL.elf ]; then
    wget ${DOWNLOAD_URL}FSBL.elf ${DOWNLOAD_ARGS}
  fi
  
  cd ../../../
  
  # Check if bitstream exists - otherwise download it
  cd sdk
  
  if [ ! -f bigpulp-z-70xx.bit ]; then
    wget ${DOWNLOAD_URL}bigpulp-z-70xx.bit ${DOWNLOAD_ARGS}
  fi
  
  cd ..
  
  # Check if BOOT.bif exists - otherwise download it
  cd sdk
  
  if [ ! -f BOOT.bif ]; then
    wget ${DOWNLOAD_URL}BOOT.bif ${DOWNLOAD_ARGS}
    perl -i -pe "s|/.*/sdk|$(pwd)|" BOOT.bif # fix absolute paths inside BOOT.bif
  fi
  
  cd ..
  
  # Build BOOT.bin
  cd sdk
  bootgen -image BOOT.bif -arch zynq -o ../sd_image/BOOT.bin -w

  if [ $? -ne 0 ]; then
    echo "ERROR: boogen failed, aborting now."
    exit 1
  fi

  cd ..

else
  # Download BOOT.bin
  cd sd_image
  wget ${DOWNLOAD_URL}BOOT.bin ${DOWNLOAD_ARGS}

  if [ $? -ne 0 ]; then
    echo "ERROR: Download of BOOT.bin failed, aborting now."
    exit 1
  fi

  cd ..
fi

#################################################
#
# Buildroot root filesystem
#
#################################################

cd buildroot

echo "-----------------------------------------"
echo "-   Preparing Buildroot configuration   -"
echo "-----------------------------------------"

# Shall we use the compiler provided by Xilinx Vivado?
USE_VIVADO_TOOLCHAIN=`echo "${KERNEL_CROSS_COMPILE}" | grep -c xilinx`
PREFIX=`echo $KERNEL_CROSS_COMPILE | cut -d- -f-3`

if [ "${USE_VIVADO_TOOLCHAIN}" -eq 1 ]; then

  # Use Xilinx Vivado toolchain - this one has no hard float support

  # Make sure we checkout a version that can be compiled with the Vivado toolchain
  # We first must get the full repo
  echo "I need to get the full repo. This will take some time."
  git fetch --unshallow
  git config remote.origin.fetch "+refs/heads/*:refs/remotes/origin/*"
  git fetch origin

  # Do the checkout
  git checkout tags/2017.05

  # Set external toolchain variables for Vivado 2017.2 GCC 4.9
  EXT_TOOLCHAIN_GCC_VERSION="4_9"
  EXT_TOOLCHAIN_KERNEL_HEADER_SERIES="3_19"

  # Derive external toolchain path
  EXT_TOOLCHAIN_PATH=`which ${KERNEL_CROSS_COMPILE}gcc`
  if [[ ! -z "${EXT_TOOLCHAIN_PATH}" ]]; then
    EXT_TOOLCHAIN_PATH=`echo "${EXT_TOOLCHAIN_PATH}" | rev | cut -d/ -f-5 --complement | rev`
    EXT_TOOLCHAIN_PATH=`echo "${EXT_TOOLCHAIN_PATH}"/arm/lin`
  else
    echo "ERROR: No cross compile toolchain found, aborting now."
    exit 1
  fi

  # Prepare the configs for Buildroot and Busybox
  take_newer buildroot-config_2017.05 .config
  take_newer busybox-config_2017.05   busybox-config

else

  # Use GCC toolchain with hard float support

  ## Use Linaro GCC toolchain - there is an issue with our own compiler and Buildroot
  #cd ..
  #mkdir linaro_gcc
  #cd linaro_gcc
  #
  ## Download and prepare Linaro GCC 7
  #wget https://releases.linaro.org/components/toolchain/binaries/7.1-2017.08/arm-linux-gnueabihf/gcc-linaro-7.1.1-2017.08-x86_64_arm-linux-gnueabihf.tar.xz --no-verbose -N
  #tar xf gcc-linaro-7.1.1-2017.08-x86_64_arm-linux-gnueabihf.tar.xz
  #mv gcc-linaro-7.1.1-2017.08-x86_64_arm-linux-gnueabihf arm-linux-gnueabihf
  #
  ## Configure PATH
  #PATH=`realpath arm-linux-gnueabihf/bin`:${PATH}
  #cd ../buildroot

  # Set external toolchain variables for GCC 7.1
  EXT_TOOLCHAIN_GCC_VERSION="7"
  EXT_TOOLCHAIN_KERNEL_HEADER_SERIES="4_9"

  # Dervie external toolchain path
  EXT_TOOLCHAIN_PATH=`which ${KERNEL_CROSS_COMPILE}gcc`
  if [[ ! -z "${EXT_TOOLCHAIN_PATH}" ]]; then
    EXT_TOOLCHAIN_PATH=`echo "${EXT_TOOLCHAIN_PATH}" | rev | cut -d/ -f-2 --complement | rev`
  else
    echo "ERROR: No cross compile toolchain found, aborting now."
    exit 1
  fi

  # Prepare the configs for Buildroot and Busybox
  take_newer buildroot-config_2017.11.2 .config
  take_newer busybox-config_2017.11.2   busybox-config

fi

# Adjust configuration with exact toolchain path
sed -i "/^BR2_TOOLCHAIN_EXTERNAL_PATH=/c\BR2_TOOLCHAIN_EXTERNAL_PATH=\"${EXT_TOOLCHAIN_PATH}\"" .config

# Adjust configuration with GCC version
sed -i "s/\(BR2_TOOLCHAIN_EXTERNAL_GCC_[0-9]\)=y/# \1 is not set/" .config
sed -i "s/\(BR2_TOOLCHAIN_EXTERNAL_GCC_[0-9]_[0-9]*\)=y/# \1 is not set/" .config
sed -i "/^# BR2_TOOLCHAIN_EXTERNAL_GCC_${EXT_TOOLCHAIN_GCC_VERSION}/c\BR2_TOOLCHAIN_EXTERNAL_GCC_${EXT_TOOLCHAIN_GCC_VERSION}=y" .config

# Adjust configuration with headers of the used GCC
sed -i "s/\(BR2_TOOLCHAIN_EXTERNAL_HEADERS_[0-9]\)=y/# \1 is not set/" .config
sed -i "s/\(BR2_TOOLCHAIN_EXTERNAL_HEADERS_[0-9]_[0-9]*\)=y/# \1 is not set/" .config
sed -i "/^# BR2_TOOLCHAIN_EXTERNAL_HEADERS_${EXT_TOOLCHAIN_KERNEL_HEADER_SERIES}/c\BR2_TOOLCHAIN_EXTERNAL_HEADERS_${EXT_TOOLCHAIN_KERNEL_HEADER_SERIES}=y" .config

# Adjust configuration with external toolchain prefix
sed -i "/^BR2_TOOLCHAIN_EXTERNAL_PREFIX/c\BR2_TOOLCHAIN_EXTERNAL_PREFIX=\"${PREFIX}\"" .config
sed -i "/^BR2_TOOLCHAIN_EXTERNAL_CUSTOM_PREFIX/c\BR2_TOOLCHAIN_EXTERNAL_CUSTOM_PREFIX=\"${PREFIX}\"" .config

# Backup and clear LD_LIBRARY_PATH
LD_LIBRARY_PATH_BKP=${LD_LIBARY_PATH}
unset LD_LIBRARY_PATH

# Setup Busybox
if [ ! -d output/build/busybox-*/ ]; then
  ./setup_busybox.sh
fi

# Build root filesystem
./generate_fs.sh ${HERO_PARALLEL_BUILD}

# The acl package might require a patch on some host machines (it is needed for the host)
if [ $? -ne 0 ]; then

  echo "Buildroot build failed as expected. I first need to patch acl.mk."

  # Apply the patch
  patch package/acl/acl.mk < acl.mk.patch

  # Delete build folder
  rm -rf output/build/host-acl*

  # Re-try to build
  ./generate_fs.sh ${HERO_PARALLEL_BUILD}
fi

if [ $? -ne 0 ]; then
  echo "ERROR: generate_fs.sh failed, aborting now."
  exit 1
fi

export LD_LIBRARY_PATH=${LD_LIBARY_PATH_BKP}
unset OLD_LD_LIBRARY_PATH

cd ..

#################################################
#
# Root filesystem customization
#
#################################################

cd root_filesystem

# Customize
./customize_fs.sh

if [ $? -ne 0 ]; then
  echo "ERROR: customize_fs.sh failed, aborting now."
  exit 1
fi

# Add U-Boot header
./add_header.sh

if [ $? -ne 0 ]; then
  echo "ERROR: add_header.sh failed, aborting now."
  exit 1
fi

cd ..

echo "Linux boot files sucessfully generated or downloaded!"
