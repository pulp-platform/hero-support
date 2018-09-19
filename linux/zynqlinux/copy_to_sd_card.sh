#!/bin/bash

# Copy boot files image to SD card.

if [[ -z "${SD_BOOT_PARTITION}" ]]; then
  SD_BOOT_PARTITION="/run/media/${USER}/ZYNQ_BOOT"
fi

# Check if boot partition of the SD card is mounted.
grep -qs "$SD_BOOT_PARTITION " /proc/mounts
not_mounted=$?
if [ "$not_mounted" -eq "1" ]; then
  echo "Error: Boot partition of SD card is not mounted at '${SD_BOOT_PARTITION}!"
  echo "Please make sure it is mounted and adjust 'SD_BOOT_PARTITION' to its mount point."
  exit 1
fi

# Copy to SD card
cp BOOT.bin devicetree.dtb uImage uramdisk.image.gz ${SD_BOOT_PARTITION}/.

# Unmount SD card
sync
umount ${SD_BOOT_PARTITION}
