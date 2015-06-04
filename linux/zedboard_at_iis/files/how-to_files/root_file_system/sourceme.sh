#!/bin/bash

# machine where you have root privileges
export SCP_TARGET_MACHINE="pirmin@iis-ubuntu2.ee.ethz.ch"

# base path to which the move_to_root.sh scripts transfers the root file system for customization
export SCP_TARGET_PATH="~/pulp_on_fpga/"