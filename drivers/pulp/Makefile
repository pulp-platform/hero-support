obj-m := pulp.o
pulp-objs := pulp_module.o pulp_mem.o pulp_rab.o pulp_dma.o

ARCH := arm
CROSS_COMPILE := arm-xilinx-linux-gnueabi-

KERNEL_DIR := $(shell echo $(KERNEL_DIR))
ARM_INC_DIR1 := $(shell echo $(ARM_INC_DIR1))
PULP_INC_DIR1 := $(shell echo $(PULP_INC_DIR1))

ccflags-y := -I${ARM_INC_DIR1}
ccflags-y += -I${PULP_INC_DIR1}

all: modules
modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(shell pwd) modules

clean:
	rm -f modules.order
	rm -f pulp.mod.*
	rm -f *.o
	rm -f pulp.ko
	rm -f Module.symvers