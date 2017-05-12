#!/bin/bash

# Generate the BOOT.BIN to be placed on the SD card.

# Copy U-Boot and FSBL for TE0808
cp ${PREBUILT_PATH}/u-boot.elf      ${IMAGE_PATH}/.
cp ${PREBUILT_PATH}/zynqmp_fsbl.elf ${IMAGE_PATH}/.

# Copy bitstream
cp ${BITSTREAM_PATH}/zusys_wrapper.bit ${IMAGE_PATH}/.

# Go to iamge folder and generate
cd ${IMAGE_PATH}
petalinux-package --boot --fsbl zynqmp_fsbl.elf --fpga zusys_wrapper.bit --u-boot u-boot.elf --force
