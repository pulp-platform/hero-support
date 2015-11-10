#!/bin/bash

# Download sources

# create the source folder
mkdir -p ${ARM_LIB_EXT_DIR}/../src

# go to source folder
cd ${ARM_LIB_EXT_DIR}/../src

# FFmpeg
git clone git://source.ffmpeg.org/ffmpeg.git ffmpeg
cd ffmpeg
#git tag -l
git checkout tags/n2.7.2
cd ..

# OpenCV
git clone https://github.com/Itseez/opencv.git
cd opencv
#git tag -l
git checkout tags/3.0.0