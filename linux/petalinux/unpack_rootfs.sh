#!/bin/bash

# This script can be used to unpack and inspect the current root file system.

# Enter fakeroot environment
if [[ $EUID -ne 0 ]]
then
    echo "Entering fakeroot environment..."
    fakeroot $0
else
    # Info
    echo "-----------------------------------------"
    echo "-        Customizing File System        -"
    echo "-----------------------------------------"
		
		# Copy the pre-genererated root file system
		cp ${IMAGE_PATH}/rootfs.cpio.gz .
		
		# Make sure tmp_mnt exists
		if [ ! -d tmp_mnt ]
		then
		    mkdir tmp_mnt
		fi 
    
    # Create a copy of the custom files
    cp -r files files_to_add
    
    # Change owner of files_to_add to root
    chown -R root:root files_to_add
    
    # Unpack the root file system to tmp_mnt/
    gunzip -c rootfs.cpio.gz | sh -c 'cd tmp_mnt/ && cpio -i'
fi
