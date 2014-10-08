#!/bin/bash

# Make
vivado-2014.1 make

# Copy
scp *.ko pirmin@iis-ubuntu2.ee.ethz.ch:~/PULPonFPGA/share/driver/.
