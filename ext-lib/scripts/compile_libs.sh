#!/bin/bash

# Compile script for the external ARM libraries
# Call using vivado-2015.1 ./compile_libs.sh 

OLD_DIR=`pwd`
cd ${ARM_LIB_EXT_DIR}/../src/

# FFmpeg
cd ffmpeg

echo "Configuring FFmpeg"
./configure --enable-shared --disable-static --cross-prefix=arm-xilinx-linux-gnueabi- --arch=armv7l --target-os=linux --prefix=${ARM_LIB_EXT_DIR}

echo "Building FFmpeg"
make -j${N_CORES_COMPILE}
make install

cd ..

# OpenCV
cd opencv
cp ${OLD_DIR}/../configs/arm-xilinx-linux-gnueabi.toolchain.cmake platforms/linux/.
echo "\n" >> platforms/linux/arm-xilinx-linux-gnueabi.toolchain.cmake
echo "set( CMAKE_INSTALL_PREFIX ${ARM_LIB_EXT_DIR} )" >> platforms/linux/arm-xilinx-linux-gnueabi.toolchain.cmake 
echo "set( CMAKE_FIND_ROOT_PATH ${ARM_LIB_EXT_DIR} ${ARM_LIB_EXT_DIR}/../release ${WORKSPACE_DIR}/buildroot/output/target/usr )" >> platforms/linux/arm-xilinx-linux-gnueabi.toolchain.cmake

export PKG_CONFIG_LIBDIR=${ARM_LIB_EXT_DIR}/lib/pkgconfig
export PKG_CONFIG_PATH=${ARM_LIB_EXT_DIR}/lib/pkgconfig

cd ..
cd ..
mkdir release
cd release

echo "Configuring OpenCV"
cmake -D CMAKE_TOOLCHAIN_FILE=platforms/linux/arm-xilinx-linux-gnueabi.toolchain.cmake -D BUILD_opencv_nonfree=OFF ${ARM_LIB_EXT_DIR}/../src/opencv

echo "make sure BZIP2_LIBRARIES points to"
echo "${WORKSPACE_DIR}/buildroot/output/target/usr/lib/libbz2.so"
echo ""
echo "switch to ON the following items"
echo "BUILD_ZLIB"
echo "BUILD_PNG"
echo ""
echo "switch to OFF the following items"
echo "WITH_1394"
echo "WITH_CUDA"
echo "WITH_CUFFT"
echo "WITH_EIGEN"
echo "WITH_GSTREAMER"
echo "WITH_GTK"
echo "WITH_JASPER"
echo "WITH_JPEG"
echo "WITH_OPENEXR"
echo "WITH_PVAPI"
echo "WITH_QT"
echo "WITH_TBB"
echo "WITH_TIFF"
echo "WITH_UNICAP"
echo "WITH_V4L"
echo "WITH_XINE"

ccmake .

# manually copy ffmpeg libraries (somehow, cmake does not manage to tell the linker where to find them)
cp ${ARM_LIB_EXT_DIR}/lib/libavcodec.so.56 lib/.
cp ${ARM_LIB_EXT_DIR}/lib/libavformat.so.56 lib/.
cp ${ARM_LIB_EXT_DIR}/lib/libavutil.so.54 lib/.
cp ${ARM_LIB_EXT_DIR}/lib/libswscale.so.3 lib/.
cp ${ARM_LIB_EXT_DIR}/lib/libswresample.so.1 lib/.

# manually copy the bz2 library from the buildroot folder (somehow, cmake does not manage to tell the linker where to find them)
cp ${WORKSPACE_DIR}/buildroot/output/target/usr/lib/libbz2.so.1.0 lib/. 

echo "Building OpenCV"
make -j${N_CORES_COMPILE}
make install
cd ..

# copy to target
scp -r lib ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/.