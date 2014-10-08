cmd_/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o := arm-xilinx-linux-gnueabi-gcc -Wp,-MD,/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/.v7_pmu.o.d  -nostdinc -isystem /usr/pack/vivado-2014.1-kgf/SDK/2014.1/gnu/arm/lin/bin/../lib/gcc/arm-xilinx-linux-gnueabi/4.8.1/include -I/scratch/vogelpi/PULPonFPGA/WorkSpace/SW/linux-xlnx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/scratch/vogelpi/PULPonFPGA/WorkSpace/SW/linux-xlnx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/scratch/vogelpi/PULPonFPGA/WorkSpace/SW/linux-xlnx/include/uapi -Iinclude/generated/uapi -include /scratch/vogelpi/PULPonFPGA/WorkSpace/SW/linux-xlnx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float       -DMODULE  -c -o /usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o /usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.S

source_/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o := /usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.S

deps_/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o := \
  /scratch/vogelpi/PULPonFPGA/WorkSpace/SW/linux-xlnx/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o: $(deps_/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o)

$(deps_/usr/ela/home/vogelpi/pulp_on_fpga/software/arm/drivers/zynq_pmm/v7_pmu.o):
