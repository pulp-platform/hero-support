#!/bin/bash

source env.sh

TARGET=`echo ${HERO_TARGET_HOST} | rev | cut -d@ -f-1 | rev`
ping ${TARGET} -c 1 > /dev/null
if [ $? -ne 0 ]; then
  echo "ERROR: HERO target host ${TARGET} not online, aborting now."
  exit 1
fi

cd ${JUNO_WORKSTATION_TARGET_PATH}

# remove symlinks from /lib/modules/
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

# copy modules
scp -r custom_files/lib/modules/* ${HERO_TARGET_HOST}:/lib/modules/.

echo "Copied all stuff to ${TARGET}."
