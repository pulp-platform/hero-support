#!/bin/bash

set -e

readonly DEV_PATH='/dev/sdb1'
readonly DEV_UUID='EA98-113D'
readonly MNT_PATH="$HOME/tmp_mnt"
readonly TIMESTAMP="$(date +%Y-%m-%d_%H-%M-%S)"
readonly BKP_PATH="$HOME/Backups/bitstreams/pre_$TIMESTAMP"
readonly BITSTREAM_SUBDIR='SITE2/HBI0247C/BIGPULP'

echoerr() { echo "$@" 1>&2; }

echoerr "[!!] Open a 'minicom' to 'ttyUSB1' and perform the following commands:"
echoerr "[!!] flash; eraseall; quit; usb_on"
echoerr -n "[!!] Hit any key when you are done."
read x

sudo blkid | grep "^$DEV_PATH:.*UUID=\"$DEV_UUID\".*$" > /dev/null
if [[ "$?" -ne "0" ]]; then
	echo "[EE] Device at '$DEV_PATH' does not match UUID '$DEV_UUID'!";
	exit 1;
fi

sudo mount "$DEV_PATH" "$MNT_PATH"
echo "[II] Mounted '$DEV_PATH' at '$MNT_PATH'."

mkdir -p "$BKP_PATH"
echo "[II] Creating backup of old bitstream at '$BKP_PATH' .."
cp -r ${MNT_PATH}/${BITSTREAM_SUBDIR}/* ${BKP_PATH}/
echo "[II] .. done."

echo "[II] Writing new bitstream to Juno .."
sudo cp -r "$HOME/juno/imgs/bitstream/bigpulp/$BITSTREAM_SUBDIR" "$MNT_PATH/$BITSTREAM_SUBDIR/../"
echo "[II] .. done."

echo "[II] Synchronizing flash drive .."
sync
echo "[II] .. done."
sudo umount "$MNT_PATH"
echo "[II] Unmounted '$DEV_PATH'."

echoerr "[!!] Open a 'minicom' to 'ttyUSB1' and enter 'usb_off'."
