#!/bin/sh
# Mount second partition on SD card if not mounted yet.
if ! grep -qs '/mnt/storage ' /proc/mounts; then
    mount /dev/mmcblk0p2 /mnt/storage
fi
