#!/bin/sh

if [ $# -gt 0 ]
then
    BITSTREAM=$1
else
    BITSTREAM=bigpulp_z_70xx_top.bit
fi

cat $BITSTREAM > /dev/xdevcfg

cat /sys/class/xdevcfg/xdevcfg/device/prog_done
