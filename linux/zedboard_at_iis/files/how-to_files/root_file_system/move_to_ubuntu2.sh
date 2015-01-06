#!/bin/bash
#scp -r * pirmin@iis-ubuntu2:~/pulp_on_fpga/root_file_system/.
scp -r custom_files/ pirmin@iis-ubuntu2:~/pulp_on_fpga/root_file_system/.
scp -r tmp_mnt/ pirmin@iis-ubuntu2:~/pulp_on_fpga/root_file_system/.
scp customize_fs.sh pirmin@iis-ubuntu2:~/pulp_on_fpga/root_file_system/.
scp rootfs.cpio.gz pirmin@iis-ubuntu2:~/pulp_on_fpga/root_file_system/.
