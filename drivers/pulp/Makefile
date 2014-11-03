obj-m := pulp.o

ARCH := arm
CROSS_COMPILE := arm-xilinx-linux-gnueabi-

KERNELDIR := /scratch/vogelpi/zc706/workspace/linux-xlnx

all: modules
modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	rm -f modules.order
	rm -f pulp.mod.*
	rm -f pulp.o
	rm -f pulp.ko
	rm -f Module.symvers