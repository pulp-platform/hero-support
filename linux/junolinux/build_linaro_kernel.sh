#!/bin/bash

# Make sure required variables are set
if [ -z "${KERNEL_ARCH}" ] || [ -z "${KERNEL_CROSS_COMPILE}" ] ; then
  echo "ERROR: Missing required environment variables KERNEL_ARCH and/or KERNEL_CROSS_COMPILE, aborting now."
  exit 1;
fi

cd workspace

# Prepare environment
export TYPE=oe
export CROSS_COMPILE=$KERNEL_CROSS_COMPILE

if [ ${LINARO_RELEASE} == "16.03" ]; then
  BUILD_ARGS=juno-${TYPE}
  KERNEL_OUT="out/${BUILD_ARGS}"
else
  BUILD_ARGS="-p juno -t juno -f oe"
  KERNEL_OUT="out/juno/mobile_oe"
fi
export KERNEL_OUT

# Do it!
./build-scripts/build-all.sh ${BUILD_ARGS} clean

./build-scripts/build-all.sh ${BUILD_ARGS} build
if [ $? -ne 0 ]; then
  echo "ERROR: build_all.sh for ${BUILD_ARGS} build failed, aborting now."
  exit 1
fi

./build-scripts/build-all.sh ${BUILD_ARGS} package
if [ $? -ne 0 ]; then
  echo "ERROR: build_all.sh for ${BUILD_ARGS} package failed, aborting now."
  exit 1
fi

cd ..

# Compile modules
cd workspace/linux/
./compile_modules.sh
