#!/bin/bash

# delete symlinks in /lib/modules/
cd custom_files/lib/modules
for DIR in * ; do
    if [ -d "${DIR}" ]; then
	cd ${DIR}
	if [ -h build ]; then
	    echo "Deleting symbolic link ${PWD}/build"
	    rm build
	fi
	if [ -h source ]; then
	    echo "Deleting symbolic link ${PWD}/source"
	    rm source
	fi
	cd ..
    fi
done
cd ../../..

# copy files
echo "Copying files to ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/"
scp -r custom_files/ ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/.
scp -r tmp_mnt/      ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/.
scp customize_fs.sh  ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/.
scp rootfs.cpio.gz   ${SCP_TARGET_MACHINE}:${SCP_TARGET_PATH}/root_file_system/.
