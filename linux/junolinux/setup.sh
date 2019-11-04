#!/bin/bash

# Project setup script for Juno Linux HOWTO

# Make sure everyting is configured and that we have the correct environment variables
source env.sh

# Info
echo "-----------------------------------------"
echo "-    Executing project setup script     -"
echo "-----------------------------------------"

# Linaro GCC
if [ ! -d "gcc-linaro" ]; then

  # Create directory
  mkdir gcc-linaro
  cd gcc-linaro

  # Download and unpack
  for arch in 'aarch64-linux-gnu' 'arm-linux-gnueabihf'; do
    if [ ${LINARO_RELEASE} == "16.03" ]; then
      wget "http://releases.linaro.org/components/toolchain/binaries/5.2-2015.11-2/${arch}/gcc-linaro-5.2-2015.11-2-x86_64_${arch}.tar.xz";
    elif [ ${LINARO_RELEASE} == "17.04" ] || [ ${LINARO_RELEASE} == "17.10" ]; then
      wget "http://releases.linaro.org/components/toolchain/binaries/6.2-2016.11/${arch}/gcc-linaro-6.2.1-2016.11-x86_64_${arch}.tar.xz";
    else
      wget "http://releases.linaro.org/components/toolchain/binaries/7.1-2017.08/${arch}/gcc-linaro-7.1.1-2017.08-x86_64_${arch}.tar.xz";
    fi
    tar xf *${arch}.tar.xz;
    rm *${arch}.tar.xz;
    mv *${arch} ${arch};
  done

  cd ..
fi

# Python Libs
python -c "import Crypto"
if [ $? -ne 0 ]; then
  if [ ! -d ${PYTHON_LIBS_LOCAL}/lib64/python/Crypto ]; then

    # Download
    wget https://pypi.python.org/packages/source/p/pycrypto/pycrypto-2.6.1.tar.gz
    tar xf pycrypto-2.6.1.tar.gz
    cd pycrypto-2.6.1/
    python setup.py build

    # Install
    if [ ! -d ${PYTHON_LIBS_LOCAL} ]; then
      mkdir -p ${PYTHON_LIBS_LOCAL}
    fi
    python setup.py install --home=${PYTHON_LIBS_LOCAL}

    cd ..

    # Remove files
    rm pycrypto-2.6.1.tar.gz
    rm -rf pycrypto-2.6.1
  fi
fi
python -c "import wand.image"
if [ $? -ne 0 ]; then
  if [ ! -f ${PYTHON_LIBS_LOCAL}/lib/python/Wand-0.4.4* ]; then

    # Download
    wget https://files.pythonhosted.org/packages/c5/0e/4c7846ffac7a478578ff77c93d6aff3da2c181972d9447c74bfe1e87ac06/Wand-0.4.4.tar.gz
    tar xf Wand-0.4.4.tar.gz
    cd Wand-0.4.4/
    python setup.py build

    # Install
    if [ ! -d ${PYTHON_LIBS_LOCAL} ]; then
      mkdir -p ${PYTHON_LIBS_LOCAL}
    fi
    python setup.py install --home=${PYTHON_LIBS_LOCAL}

    cd ..

    # Remove files
    rm Wand-0.4.4.tar.gz
    rm -rf Wand-0.4.4
  fi
fi

# Repo tool
if [ ! -f "${BIN_LOCAL}/repo" ]; then
  if [ ! -d "${BIN_LOCAL}" ]; then
    mkdir -p ${BIN_LOCAL}
  fi

  # Download
  curl https://storage.googleapis.com/git-repo-downloads/repo > ${BIN_LOCAL}/repo
  chmod a+x ${BIN_LOCAL}/repo
fi

# Bitbake
if [ ! -d "${BIN_LOCAL}/bitbake" ]; then
  if [ ! -d "${BIN_LOCAL}" ]; then
    mkdir -p ${BIN_LOCAL}
  fi

  # Download
  git clone git://git.openembedded.org/bitbake ${BIN_LOCAL}/bitbake
fi

# Linaro Linux sources
if [ ! -d "linaro_${LINARO_RELEASE}" ]; then

  # Create directory
  mkdir linaro_${LINARO_RELEASE}
  cd linaro_${LINARO_RELEASE}

  # Move helper scripts and files
  mv ../config_linaro_kernel.sh .
  mv ../build_linaro_kernel.sh  .
  mv ../compile_kernel.sh       .
  mv ../compile_modules.sh      .
  mv ../menuconfig.sh           .

  mv ../pulp.dtsi*              .
  mv ../juno-base.patch*        .
  mv ../kernel-config           .

  # Download sources
  ./config_linaro_kernel.sh

  cd ..
fi

# Linaro 17.04 wants to have a local "copy" of the compiler
if [ ${LINARO_RELEASE} == "17.04" ] || [ ${LINARO_RELEASE} == "17.10" ]; then
  GCC_COPY_DIR="linaro_${LINARO_RELEASE}/workspace/tools/gcc"
  if [ ! -d "${GCC_COPY_DIR}" ]; then
    mkdir -p "${GCC_COPY_DIR}"
    ln -s `realpath gcc-linaro/aarch64-linux-gnu` "${GCC_COPY_DIR}/gcc-linaro-6.2.1-2016.11-x86_64_aarch64-linux-gnu"
    ln -s `realpath gcc-linaro/arm-linux-gnueabihf` "${GCC_COPY_DIR}/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf"
  fi
fi

# OpenEmbedded sources
if [ ! -d "oe_${OE_RELEASE}" ]; then

  # Create directory
  mkdir oe_${OE_RELEASE}
  cd oe_${OE_RELEASE}

  # Move helper scripts and files
  mv ../config_oe.sh .
  mv ../build_oe.sh  .
  mv ../source_oe.sh .

  mv ../local.conf   .

  if [ ${OE_RELEASE} == "16.03" ]; then

    mkdir ${OE_HOME}

    # Download sources
    ./config_oe.sh

    # Init build system
    OLD_PWD=`pwd`
    cd ${OE_HOME}
    ./jenkins-setup/init-and-build.sh -b release-${OE_RELEASE}
    cd ${OLD_PWD}

    # Prepare OE config
    cp local.conf openembedded/build/conf/local.conf
    echo "BB_NUMBER_THREADS = \"${BB_PARALLELISM}\"" >> openembedded/build/conf/local.conf
    echo "BB_NUMBER_PARSE_THREADS = \"${BB_PARALLELISM}\"" >> openembedded/build/conf/local.conf
    echo "PARALLEL_MAKE = \"${BB_PARALLEL_BUILD}\"" >> openembedded/build/conf/local.conf

    # Patch OE git script (--set-upstream is deprecated)
    sed -i "s|--set-upstream %s origin/%s|--set-upstream-to origin/%s %s|" openembedded/openembedded-core/bitbake/lib/bb/fetch2/git.py

    # Download the hwpack
    wget http://releases.linaro.org/openembedded/juno-lsk/16.03/hwpack_linaro-lt-vexpress64-rtsm_20160329-738_arm64_supported.tar.gz -N

  elif [ ${OE_RELEASE} == "17.01" ]; then

    # Download the precompiled image
    wget http://releases.linaro.org/openembedded/juno-lsk/17.01/lt-vexpress64-openembedded_minimal-armv8-gcc-5.2_20170127-761.img.gz -N

    # Download the hwpack
    wget http://releases.linaro.org/openembedded/juno-lsk/17.01/hwpack_linaro-lt-vexpress64-rtsm_20170127-761_arm64_supported.tar.gz -N
  else

    echo "ERROR: OpenEmbedded Release ${OE_RELEASE} not supported, aborting now."
    exit 1
  fi

  cd ..
fi
