#!/bin/bash

source ../env.sh

# Make sure required variables are set
if [ -z "${OE_RELEASE}" ] || [ -z "${OE_HOME}" ] ; then
  echo "ERROR: Missing required environment variables OE_RELEASE and/or OE_HOME, aborting now."
  exit 1;
fi

cd ${OE_HOME}
git clone git://git.linaro.org/openembedded/jenkins-setup.git

cd $OE_HOME/jenkins-setup
git checkout release-15.12
cd ..

# Setup dependencies - not needed
#sudo jenkins-setup/pre-build-root-install-dependencies.sh
