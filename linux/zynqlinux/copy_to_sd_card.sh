#!/bin/bash

# Copy boot files image to SD card.

SD_BOOT_PARTITION="/run/media/${USER}/ZYNQ_BOOT"

# Copy to SD card
cp BOOT.bin          ${SD_BOOT_PARTITION}/.
cp devicetree.dtb    ${SD_BOOT_PARTITION}/.
cp uImage            ${SD_BOOT_PARTITION}/.
cp uramdisk.image.gz ${SD_BOOT_PARTITION}/.

# Unmount SD card
sync
umount ${SD_BOOT_PARTITION}
