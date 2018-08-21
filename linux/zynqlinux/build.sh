#!/bin/bash

# Project build script for Zynq Linux HOWTO

# Info
echo "-----------------------------------------"
echo "-    Executing project build script     -"
echo "-----------------------------------------"

DOWNLOAD_URL="https://iis-people.ee.ethz.ch/~vogelpi/hero/files/images/zc706/"
DOWNLOAD_ARGS="--no-verbose -N --user=hero --password=weekly*report2018"

# Build configuration
PARBUILD=-j8

#################################################
#
# Linux kernel
#
#################################################

# Try to build kernel (might fail because of missing U-Boot mkimage)
{
  cd linux-xlnx
  ./compile_kernel.sh  ${PARBUILD}
  cd ..
} || {
    COMPILE_KERNEL_FAILED=1
}

if [[ ! -z "${COMPILE_KERNEL_FAILED}" ]]; then
  echo "Linux kernel build failed as expected. I first need to build U-Boot mkimage."

  # Build U-Boot (needed for kernel build)
  cd u-boot-xlnx
  ./compile_loader.sh ${PARBUILD}
  cd ..

  # Rebuild kernel
  cd linux-xlnx
  ./compile_kernel.sh  ${PARBUILD}

  if [ $? -ne 0 ]; then
    echo "ERROR: compile_kernel.sh failed, aborting now."
    exit 1
  fi

  cd ..
fi

# Also build modules
cd linux-xlnx
./compile_modules.sh ${PARBUILD}

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
./compile_loader.sh ${PARBUILD}
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

# Make sure KERNEL_CROSS_COMPILE is set
if [[ -z "KERNEL_CROSS_COMPILE" ]]; then
  export KERNEL_CROSS_COMPILE="arm-xilinx-linux-gnueabi-"
fi

# Shall we use the compiler provided by Xilinx Vivado?
USE_VIVADO_TOOLCHAIN=`echo "${KERNEL_CROSS_COMPILE}" | grep -c xilinx`
PREFIX=`echo $KERNEL_CROSS_COMPILE | cut -d- -f-3`

if [ "${USE_VIVADO_TOOLCHAIN}" -eq 1 ]; then

  # Use Xilinx Vivado toolchain - this one has no hard float support

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
  cp buildroot-config_2017.05 .config
  cp busybox-config_2017.05   busybox-config

else

  # Use GCC toolchain with hard float support

  # Make sure we checkout a version that can be compiled with GCC 7.1
  git checkout tags/2017.11.2

  # Use Linaro GCC toolchain - there is an issue with our own compiler and Buildroot
  cd ..
  mkdir linaro_gcc
  cd linaro_gcc

  # Download and prepare Linaro GCC 7
  wget https://releases.linaro.org/components/toolchain/binaries/7.1-2017.08/arm-linux-gnueabihf/gcc-linaro-7.1.1-2017.08-x86_64_arm-linux-gnueabihf.tar.xz --no-verbose -N
  tar xf gcc-linaro-7.1.1-2017.08-x86_64_arm-linux-gnueabihf.tar.xz
  mv gcc-linaro-7.1.1-2017.08-x86_64_arm-linux-gnueabihf arm-linux-gnueabihf

  # Configure PATH
  PATH=`realpath arm-linux-gnueabihf/bin`:${PATH}
  cd ../buildroot

  # Set external toolchain variables for Linaro GCC 7.1
  EXT_TOOLCHAIN_GCC_VERSION="7"
  EXT_TOOLCHAIN_KERNEL_HEADER_SERIES="4_10"

  # Dervie external toolchain path
  EXT_TOOLCHAIN_PATH=`which ${KERNEL_CROSS_COMPILE}gcc`
  if [[ ! -z "${EXT_TOOLCHAIN_PATH}" ]]; then
    EXT_TOOLCHAIN_PATH=`echo "${EXT_TOOLCHAIN_PATH}" | rev | cut -d/ -f-2 --complement | rev`
  else
    echo "ERROR: No cross compile toolchain found, aborting now."
    exit 1
  fi

  # Prepare the configs for Buildroot and Busybox
  cp buildroot-config_2017.11.2 .config
  cp busybox-config_2017.11.2   busybox-config

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
./generate_fs.sh ${PARBUILD}

# The acl package might require a patch on some host machines (it is needed for the host)
if [ $? -ne 0 ]; then

  echo "Buildroot build failed as expected. I first need to patch acl.mk."

  # Apply the patch
  patch package/acl/acl.mk < acl.mk.patch

  # Delete build folder
  rm -rf output/build/host-acl*

  # Re-try to build
  ./generate_fs.sh ${PARBUILD}
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
