#!/bin/bash

source env.sh

# Clean flash drive
echo "Connect the SD boot drive of the Juno ADP to this machine:"
echo "1. Do minicom ttyUSB1"
echo "2. Type flash"
echo "3. Type eraseall"
echo "3. Type quit"

# Connect flash drive
echo "Connect the SD boot drive of the Juno ADP to this machine:"
echo "1. Do minicom ttyUSB1"
echo "2. Type usb_on"

# Make sure flash drive is /dev/sdb
echo "Available partitions:"
sudo blkid

echo -en "\nIs the target partition drive /dev/sdb1? [y/n] "
read x

case $x in
[yY] | [yY][Ee][Ss] ) echo "Yes, proceeding" ;;
[nN] | [n|N][O|o] )   echo "No, abort" ; exit 1;;
*)                    echo "Invalid input" ; exit 1;;
esac

# mount it
echo "Mounting /dev/sdb1"
mount /dev/sdb1 # will be mounted to /home/juno/tmp_mnt according to fstab

mkdir -p ${JUNO_WORKSTATION_TARGET_PATH}/flash/flash

# copy directory structure
cp -r ${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/recovery/* ${JUNO_WORKSTATION_TARGET_PATH}/flash/flash/.

# patch config.txt
sed -i -e s/"MBLOG: FALSE"/"MBLOG: UART1"/g                               ${JUNO_WORKSTATION_TARGET_PATH}/flash/flash/config.txt

# copy kernel output
cp -r ${JUNO_WORKSTATION_TARGET_PATH}/linaro_${LINARO_RELEASE}/output/*   ${JUNO_WORKSTATION_TARGET_PATH}/flash/flash/SOFTWARE/.

# copy bitstream
cp -r ${JUNO_WORKSTATION_TARGET_PATH}/bitstream/bigpulp/*                 ${JUNO_WORKSTATION_TARGET_PATH}/flash/flash/.

# deploy
echo "Deploying kernel"
cp -r ${JUNO_WORKSTATION_TARGET_PATH}/flash/flash/* ~/tmp_mnt

# finish
sync
echo "Unmounting /dev/sdb1"
umount /dev/sdb1

# Disconnect flash drive
echo "Disconnect the SD boot drive of the Juno ADP from this machine:"
echo "1. Do minicom ttyUSB1"
echo "2. Type usb_off"
