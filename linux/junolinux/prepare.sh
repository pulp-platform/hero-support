#!/bin/bash

source env.sh

TARGET=`echo ${JUNO_WORKSTATION_TARGET_HOST} | rev | cut -d@ -f-1 | rev`
ping ${TARGET} -c 1 > /dev/null
if [ $? -ne 0 ]; then
  echo "ERROR: Juno workstation target ${TARGET} not online, aborting now."
fi

if [ ${LINARO_RELEASE} == "16.03" ]; then
  KERNEL_OUT="linaro_${LINARO_RELEASE}/workspace/linux/out/juno-oe"
  LINARO_OUT="linaro_${LINARO_RELEASE}/workspace/output/juno-oe"
else
  KERNEL_OUT="linaro_${LINARO_RELEASE}/workspace/linux/out/juno/mobile_oe"
  LINARO_OUT="linaro_${LINARO_RELEASE}/workspace/output/juno/juno-oe"
fi
export KERNEL_OUT

#################
# Boot Files
#################

# create directories
ssh ${JUNO_WORKSTATION_TARGET_HOST} "mkdir -p ${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/recovery; mkdir -p ${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/output"

# recovery stuff
scp -r linaro_${LINARO_RELEASE}/workspace/recovery/*   ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/recovery/.

# kernel stuff
scp -r ${LINARO_OUT}/uboot/*                           ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/output/.

# recompiled kernel stuff
scp -r ${KERNEL_OUT}/arch/arm64/boot/Image             ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/output/.
scp -r ${KERNEL_OUT}/arch/arm64/boot/dts/arm/juno*.dtb ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/output/.

#################
# Bitstream
#################

if [ -d bitstream/bigpulp/SITE2 ]; then
  scp -r bitstream                                     ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/.
fi

##################
# Root Filesystem
##################

# create directories
ssh ${JUNO_WORKSTATION_TARGET_HOST} "mkdir -p ${JUNO_WORKSTATION_TARGET_PATH}/oe_${OE_RELEASE}/; mkdir -p ${JUNO_WORKSTATION_TARGET_PATH}/custom_files"

# rootfs image
if [ ${OE_RELEASE} == "16.03" ]; then
  scp oe_${OE_RELEASE}/openembedded/build/tmp-glibc/deploy/images/genericarmv8/linaro_image-minimal-genericarmv8.tar.gz ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/oe_${OE_RELEASE}/.
else
  scp oe_${OE_RELEASE}/lt-vexpress64-openembedded_minimal-armv8-gcc-5.2_20170127-761.img.gz                             ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/oe_${OE_RELEASE}/.
fi

# hwpack
scp oe_${OE_RELEASE}/hwpack* ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/oe_${OE_RELEASE}/.

# kernel modules
OLD_PWD=`pwd`
cd ${KERNEL_OUT}/lib_modules/
tar cf - ./ | ssh ${JUNO_WORKSTATION_TARGET_HOST} "cd ${JUNO_WORKSTATION_TARGET_PATH}/custom_files; tar xf -"
cd ${OLD_PWD}

##################
# Scripts
##################
scp env.sh  ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/.
scp deploy* ${JUNO_WORKSTATION_TARGET_HOST}:${JUNO_WORKSTATION_TARGET_PATH}/.

echo "Copied all stuff to ${TARGET}."
