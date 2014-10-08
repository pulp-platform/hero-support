obj-m += zynq_pmm.o
zynq_pmm-objs := zynq_pm_module.o v7_pmu.o

# Cross compilation stuff
ARCH=arm
CROSS_COMPILE=arm-xilinx-linux-gnueabi-

# Paths
KERNELDIR=/scratch/vogelpi/PULPonFPGA/WorkSpace/SW/linux-xlnx

all: modules
modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	rm -f *.mod.*
	rm -f *.o
	rm -f *.ko
	rm -f Module.symvers
	rm -f modules.order
