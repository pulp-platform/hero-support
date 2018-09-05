#!/bin/bash

# Project setup script for Zynq Linux HOWTO
XILINX_TAG="xilinx-v2017.2"
BUILDROOT_TAG="2017.11.2"

# Info
echo "-----------------------------------------"
echo "-    Executing project setup script     -"
echo "-----------------------------------------"

# Device tree sources
if [ ! -d "device-tree-xlnx" ]; then

  # clone GIT repository
  git clone --branch ${XILINX_TAG} --depth 1 git://github.com/Xilinx/device-tree-xlnx.git

fi

# Linux kernel sources
if [ ! -d "linux-xlnx" ]; then

  # clone GIT repository
  git clone --branch ${XILINX_TAG} --depth 1 git://github.com/Xilinx/linux-xlnx.git

  # move helper scripts and files
  mv compile_kernel.sh   linux-xlnx/.
  mv compile_modules.sh  linux-xlnx/.
  mv generate_dtb.sh     linux-xlnx/.
  mv kernel-config       linux-xlnx/.
  mv system-top.patch    linux-xlnx/.
  mv pulp.dtsi           linux-xlnx/.
  mv cpu_opps_zc706.dtsi linux-xlnx/.
  mv cpu_opps_zed.dtsi   linux-xlnx/.

  # prepare config
  cp linux-xlnx/kernel-config linux-xlnx/.config

fi

# U-Boot sources
if [ ! -d "u-boot-xlnx" ]; then

  # clone GIT repository
  git clone --branch ${XILINX_TAG} --depth 1 git://github.com/Xilinx/u-boot-xlnx.git

  # move helper scripts and files
  mv compile_loader.sh u-boot-xlnx/.

fi

# Buildroot sources
if [ ! -d "buildroot" ]; then

  # clone GIT repository
  git clone --branch ${BUILDROOT_TAG} --depth 1 git://git.buildroot.net/buildroot.git

  # move helper scripts and files
  mv generate_fs.sh             buildroot/.
  mv setup_busybox.sh           buildroot/.
  mv sync_busybox.sh            buildroot/.
  mv buildroot-config_2017.05   buildroot/.
  mv busybox-config_2017.05     buildroot/.
  mv buildroot-config_2017.11.2 buildroot/.
  mv busybox-config_2017.11.2   buildroot/.
  mv acl.mk.patch               buildroot/.

  # prepare default configs
  cp buildroot/buildroot-config_2017.11.2 buildroot/.config
  cp buildroot/busybox-config_2017.11.2   buildroot/busybox-config

  # make sure build.sh will use the proper configs
  touch buildroot/buildroot-config_*
  touch buildroot/busybox-config_*

fi

# Root Filesystem stuff
if [ ! -d "root_filesystem" ]; then

  # create the folder
  mkdir root_filesystem

  # move helper scripts and files
  mv custom_files    root_filesystem/.
  mv customize_fs.sh root_filesystem/.
  mv add_header.sh   root_filesystem/.

fi

# SD image
if [ ! -d "sd_image" ]; then

  # create the folder
  mkdir sd_image

  # move helper scripts and files
  mv copy_to_sd_card.sh sd_image/.

fi

# SDK
if [ ! -d "sdk" ]; then

  # create the folder
  mkdir sdk

fi
