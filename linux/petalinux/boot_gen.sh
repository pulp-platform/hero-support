#!/bin/bash

# Generate the BOOT.BIN to be placed on the SD card.

# Copy U-Boot, FSBL, ARM Trusted Firmware for TE0808
cp ${PREBUILT_PATH}/os/petalinux/default/u-boot.elf                ${IMAGE_PATH}/.
cp ${PREBUILT_PATH}/software/te0808_2es2_tebf0808/zynqmp_fsbl.elf  ${IMAGE_PATH}/.
cp ${PREBUILT_PATH}/software/te0808_2es2_tebf0808/bl31.elf         ${IMAGE_PATH}/.

# Alternatively, patch the boot loader before the build
#cp -f ${WORKSPACE_PATH}/os/petalinux/components/bootloader/zynqmp_fsbl/xfsbl_board.te0808 ${WORKSPACE_PATH}/os/wimmis02/components/bootloader/zynqmp_fsbl/xfsbl_board.c

# Copy PMU firmware
cp ${PREBUILT_PATH}/software/te0808_2es2_tebf0808/zynqmp_pmufw.elf ${IMAGE_PATH}/.

# Copy bitstream
cp ${BITSTREAM_PATH}/bigpulp_zux_top.bit                           ${IMAGE_PATH}/.

# Go to image folder and generate
cd ${IMAGE_PATH}
petalinux-package --boot --fsbl zynqmp_fsbl.elf --fpga bigpulp_zux_top.bit --u-boot u-boot.elf --pmufw zynqmp_pmufw.elf --force
