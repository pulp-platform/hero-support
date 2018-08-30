obj-m := pulp.o

PLATFORM      := ${shell echo ${PLATFORM}}

ARCH          := ${shell echo ${KERNEL_ARCH}}
CROSS_COMPILE := ${shell echo ${KERNEL_CROSS_COMPILE}}

KERNEL_DIR    := ${shell echo ${KERNEL_DIR}}

HERO_TARGET_HOST         := ${shell echo ${HERO_TARGET_HOST}}
HERO_TARGET_PATH_DRIVERS := ${shell echo ${HERO_TARGET_PATH_DRIVER}}

ccflags-y += -DPLATFORM=${PLATFORM}
ccflags-y += -O2

pulp-objs := pulp_module.o pulp_mem.o pulp_rab.o pulp_dma.o pulp_mbox.o

ifeq ($(KERNEL_ARCH),arm64)
	# JUNO and ZynqMP require the stripped down pulp_cache.S
	pulp-objs += pulp_cache.o
endif

ifeq ($(PLATFORM),5)
	# ZynqMP requires pulp_smmu.*
	pulp-objs += pulp_smmu.o
endif

.PHONY: all build deploy dis

all: deploy

build: modules

modules: $(patsubst %.o,%.c,$(pulp-objs)) $(patsubst %.o,%.h,$(pulp-objs))
	$(MAKE) -S ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(shell pwd) modules

deploy: build
	rsync -v pulp.ko $(HERO_TARGET_HOST):$(HERO_TARGET_PATH_DRIVERS)/

dis: build
	$(CROSS_COMPILE)objdump -DS pulp.ko > pulp.disasm

clean:
	rm -f modules.order
	rm -f pulp.mod.*
	rm -f *.o
	rm -f pulp.ko
	rm -f Module.symvers
	rm -f .*.o.cmd
	rm -f .*.ko.cmd
	rm -f *.disasm
	rm -f *.o_shipped
