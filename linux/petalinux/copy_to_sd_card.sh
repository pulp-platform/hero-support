#!/bin/bash

# Copy boot and FIT image to SD card.

# Create sd_image folder
if [ ! -d ../sd_image ]
then
    mkdir ../sd_image
fi

# Copy to sd_image
cp ${IMAGE_PATH}/image.ub ../sd_image/.
cp ${IMAGE_PATH}/BOOT.BIN ../sd_image/.

# Copy to SD card
cp ../sd_image/* /media/TE0808_BOOT/.

# Unmount SD card
sync
umount /media/TE0808*