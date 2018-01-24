#!/bin/bash

# Generate the BOOT.BIN to be placed on the SD card.

# Copy U-Boot and ARM Trusted Firmware for TE0808 board
if [ $BOARD == te0808 ]; then
  echo "Copying prebuilt U-Boot and PMU firwmare images for TE0808"
  cp ${PREBUILT_PATH}/os/petalinux/default/u-boot.elf                ${IMAGE_PATH}/.
  cp ${PREBUILT_PATH}/software/te0808_2es2_tebf0808/zynqmp_pmufw.elf ${IMAGE_PATH}/.
fi

# Copy bitstream
cp ${VIVADO_EXPORT_PATH}/bigpulp_zux_top.bit ${IMAGE_PATH}/.

# Go to image folder and generate
cd ${IMAGE_PATH}
petalinux-package --boot --fsbl zynqmp_fsbl.elf --fpga bigpulp_zux_top.bit --u-boot u-boot.elf --pmufw zynqmp_pmufw.elf --force
