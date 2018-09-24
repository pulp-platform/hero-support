#!/bin/bash

source ../env.sh

# Make sure required variables are set
if [ -z "${OE_RELEASE}" ] || [ -z "${OE_HOME}" ] ; then
  echo "ERROR: Missing required environment variables OE_RELEASE and/or OE_HOME, aborting now."
  exit 1;
fi

# Make sure we are ready to enter the build dir
cd ${OE_HOME}

# Setup configuration - This command will move into the build dir
if [ -f "openembedded-core/oe-init-build-env" ]; then
  source openembedded-core/oe-init-build-env
else
  echo "ERROR: OpenEmbedded env scripts not found, aborting now."
fi
