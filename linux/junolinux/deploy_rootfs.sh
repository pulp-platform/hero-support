#!/bin/bash

source env.sh

# Check if
which linaro-media-create
if [ $? -eq 0 ]; then
  echo "ERROR: linaro-media-create not found, install it using sudo apt-get install linaro-media-tools, aborting now."
  exit 1
fi

# Connect drive
echo "Connect the drive of the Juno ADP to this machine."

# Make sure the drive is /dev/sdb
echo "Available partitions:"
sudo blkid

echo -en "\nIs the target drive /dev/sdb? [y/n] "
read x

case $x in
[yY] | [yY][Ee][Ss] ) echo "Yes, proceeding" ;;
[nN] | [n|N][O|o] )   echo "No, abort" ; exit 1;;
*)                    echo "Invalid input" ; exit 1;;
esac

if [ ${OE_RELEASE} == "16.03" ]; then

  # copy openembedded output
  cp ~/${JUNO_WORKSTATION_PATH}/oe_${OE_RELEASE}/linaro-image-minimal-genericarmv8.tar.gz ~/juno/imgs/rootfs/.

  # deploy
  sudo linaro-media-create \
      --mmc /dev/sdb --dev juno \
      --hwpack imgs/hwpack/hwpack_linaro-lt-vexpress64-rtsm_20160329-738_arm64_supported.tar.gz \
      --binary imgs/rootfs/linaro-image-minimal-genericarmv8.tar.gz

elif [ ${OE_RELEASE} == "17.01" ]; then

  # copy the downloaded image
  cp ~/${JUNO_WORKSTATION_PATH}/oe_${OE_RELEASE}/lt-vexpress64-openembedded_minimal-armv8-gcc-5.2_20170127-761.img.gz ~/juno/imgs/rootfs/.

  # deploy
  sudo linaro-media-create \
      --mmc /dev/sdb --dev juno \
      --hwpack imgs/hwpack/hwpack_linaro-lt-vexpress64-rtsm_20170127-761_arm64_supported.tar.gz \
      --binary imgs/rootfs/lt-vexpress64-openembedded_minimal-armv8-gcc-5.2_20170127-761.img.gz
else

  echo "ERROR: OpenEmbedded Release ${OE_RELEASE} not supported, aborting now."
  exit 1
fi

# finish
sync
sudo eject /dev/sdb

# Disconnect drive
echo "Connect the drive back to the Juno ADP."
