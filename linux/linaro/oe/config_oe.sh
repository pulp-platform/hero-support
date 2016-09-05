#!/bin/bash

mkdir openembedded && cd ./openembedded
export OE_HOME=`pwd`
git clone git://git.linaro.org/openembedded/jenkins-setup.git
cd $OE_HOME/jenkins-setup
git checkout release-${OE_RELEASE}
cd $OE_HOME
sudo jenkins-setup/pre-build-root-install-dependencies.sh
jenkins-setup/init-and-build.sh
