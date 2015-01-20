#!/bin/bash

# Name of the program
NAME=parMatrixMul

# Make
vivado-2014.1 make all

# Copy
# Host executable
scp ${NAME} pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
# Host sources for GDB
scp *.c pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
scp *.h pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.
# Accelerator binary
scp *.bin pirmin@iis-ubuntu2.ee.ethz.ch:~/pulp_on_fpga/share/programs/.