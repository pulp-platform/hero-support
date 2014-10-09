#!/bin/bash

# Name of the program
NAME=testing

# Make
vivado-2014.1 make all

# Copy
scp ${NAME} pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
scp value.txt pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
scp *.c pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
scp *.h pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
