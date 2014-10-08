#!/bin/bash

echo "------------------------------------------------"
echo "- Generation of DTB file from DTS file started -"
echo "------------------------------------------------"

# Fetching DTS file
echo "Fetching DTS file xilinx.dts from SDK project folder"
cp ../sdk/device-tree_bsp_0/ps7_cortexa9_0/libsrc/device-tree_v0_00_x/xilinx.dts arch/arm/boot/dts/.

# Converting DTS file to DTB file
echo "Converting DTS file to DTB file and copying to ../sd_image/"
./scripts/dtc/dtc -I dts -O dtb -o ../sd_image/devicetree.dtb arch/arm/boot/dts/xilinx.dts
