#!/bin/sh
# Mount NFS share on bordcomputer if not mounted yet.
if ! grep -qs '/mnt/nfs ' /proc/mounts; then
    mount -o nolock -t nfs 129.132.24.199:/home/vogelpi/pulp_on_fpga/share /mnt/nfs
fi
