#!/bin/bash

# Compile script for U-Boot
# Call using vivado-2014.1 ./compile_loader.sh

# Info
echo "-----------------------------------------"
echo "- Executing U-Boot compile script -"
echo "-----------------------------------------"

# Compile U-Boot
echo "Comiling U-Boot"
make CROSS_COMPILE=arm-xilinx-linux-gnueabi- zynq_zed_config
make CROSS_COMPILE=arm-xilinx-linux-gnueabi- $1

# Check & create sd_image directory
if [ ! -d "../boot_image" ]; then
    mkdir ../boot_image
fi

# Copy
echo "Copying u-boot to ../boot_image/u-boot.elf"
cp u-boot ../boot_image/u-boot.elf
