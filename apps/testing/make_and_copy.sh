#!/bin/bash

# Name of the program
NAME=testing

# Make
vivado-2014.1 make all

# Copy
scp ${NAME} pirmin@iis-ubuntu2.ee.ethz.ch:~/PULPonFPGA/share/programs/.
scp value.txt pirmin@iis-ubuntu2.ee.ethz.ch:~/PULPonFPGA/share/programs/.
scp *.c pirmin@iis-ubuntu2.ee.ethz.ch:~/PULPonFPGA/share/programs/.
scp *.h pirmin@iis-ubuntu2.ee.ethz.ch:~/PULPonFPGA/share/programs/.
