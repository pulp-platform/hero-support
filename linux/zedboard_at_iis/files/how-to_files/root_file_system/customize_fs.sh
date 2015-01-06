#!/bin/bash

# This script adds the content of the folder custom_files to the root
# file system
#
# You will need root privileges to execute this script!
#
# Call using ./customize_fs.sh

# Info
echo "-----------------------------------------"
echo "-        Customizing File System        -"
echo "-----------------------------------------"

# Create a copy of the root file system
cp rootfs.cpio.gz `date +%F_%H-%M-%S`_rootfs.cpio.gz

# Create a copy of the custom files
cp -r custom_files files_to_add

# Change owner of files_to_add to root
sudo chown -R root:root files_to_add

# Unpack the root file system to tmp_mnt/
gunzip -c rootfs.cpio.gz | sudo sh -c 'cd tmp_mnt/ && cpio -i'

# Move content of files_to_add to unpacked file system
sudo cp -r  files_to_add/* tmp_mnt/.

# Repack the root file system
sudo sh -c 'cd tmp_mnt/ && sudo find . | cpio -H newc -o' | gzip -9 > custom_rootfs.cpio.gz

# Clean up
sudo rm -rf files_to_add/
sudo rm -rf tmp_mnt/*
sudo mv custom_rootfs.cpio.gz rootfs.cpio.gz

# Change ownership
USER=`whoami`
sudo chown ${USER}:${USER} *.cpio.gz
