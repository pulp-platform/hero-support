#!/bin/bash

# BusyBox config sync script

echo "Syncing BusyBox configuration"

# One argument required to specify direction
if [ $# -ne 1 ]; then
  echo "ERROR: Require exactly 1 argument."
  exit 1
fi

# Go to build folder where the .config is
cd output/build/busybox-*/

# Sync...
if [ $1 -eq 0 ]; then
  # ...from Buildroot dir to BusyBox dir

  # Does the local file already exist
  if [ -f .config ]; then

    # Do the files differ? Skip the date on Line 4.
    DIFF=`diff <(tail -n +5 ../../../busybox-config) <(tail -n +5 .config)`
    if [ ! -z "${DIFF}" ]; then

      # Which one is newer?
      if test .config -nt ../../../busybox-config ; then
        # Nothing needs to be done
        echo "Continuing with current, newer .config."
      else
        # Backup .config, take newer input file
        echo "Backing up current .config, replacing by ../../../busybox-config."
        TS=`stat --printf='%.19y\n' .config`
        TS=`echo ${TS} | sed "s/ /_/"`
        TS=`echo ${TS} | sed "s/:/-/g"`
        mv .config ../../../busybox-config_${TS}
        cp ./../../busybox-config .config
      fi
    fi
  else
    cp ../../../busybox-config .config
  fi

else
  # ...from BusyBox to Buildroot dir

  # Does the remote file already exist
  if [ -f ../../../busybox-config ]; then

    # Do the files differ? Skip the date on Line 4.
    DIFF=`diff <(tail -n +5 ../../../busybox-config) <(tail -n +5 .config)`
    if [ ! -z "${DIFF}" ]; then

      # Which one is newer?
      if test ../../../busybox-config -nt .config ; then
        # Nothing needs to be done
        echo "Continuing with current, newer ../../../busybox-config."
      else
        # Take newer .config
        echo "Replacing ../../../busybox-config by newer .config."
        TS=`stat --printf='%.19y\n' .config`
        TS=`echo ${TS} | sed "s/ /_/"`
        TS=`echo ${TS} | sed "s/:/-/g"`
        cp .config ../../../busybox-config_${TS}
        cp .config ../../../busybox-config
      fi
    fi
  else
    cp .config ../../../busybox-config
  fi
fi

cd ../../../
