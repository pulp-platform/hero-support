#!/bin/bash

# Make sure required variables are set
if [ -z "${LINARO_RELEASE}" ] || [ -z "${MANIFEST}" ] ; then
  echo "ERROR: Missing required environment variables LINARO_RELEASE and/or MANIFEST, aborting now."
  exit 1;
fi

# Create workspace directory
mkdir workspace
cd workspace

# Get the sources
repo init -u https://git.linaro.org/landing-teams/working/arm/manifest -b ${LINARO_RELEASE} -m pinned-${MANIFEST}.xml
repo sync ${HERO_PARALLEL_BUILD}

cd ..

# Copy device tree snippet and patch template
cp pulp.dtsi_${LINARO_RELEASE}       workspace/linux/arch/arm64/boot/dts/arm/pulp.dtsi
cp juno-base.patch_${LINARO_RELEASE} workspace/linux/arch/arm64/boot/dts/arm/juno-base.patch

OLD_PWD=`pwd`
cd workspace/linux/arch/arm64/boot/dts/arm/
patch < juno-base.patch
cd ${OLD_PWD}

# Copy some more scripts - these can be used to re-execute the kernel build later
cp compile_kernel.sh  workspace/linux/.
cp compile_modules.sh workspace/linux/.
cp menuconfig.sh      workspace/linux/.
