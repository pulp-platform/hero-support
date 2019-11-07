obj-m := pulp.o

ifndef PLATFORM
$(error PLATFORM not defined.)
endif
ifndef KERNEL_DIR
$(error KERNEL_DIR not defined.)
endif
ifndef KERNEL_ARCH
$(error KERNEL_ARCH not defined.)
endif
ifndef KERNEL_CROSS_COMPILE
$(error KERNEL_CROSS_COMPILE not defined.)
endif
ifndef HERO_TARGET_HOST
$(error HERO_TARGET_HOST not defined.)
endif
ifndef HERO_TARGET_PATH_DRIVER
$(error HERO_TARGET_PATH_DRIVER not defined.)
endif

ARCH          := ${KERNEL_ARCH}
CROSS_COMPILE := ${KERNEL_CROSS_COMPILE}

ccflags-y += -DPLATFORM=${PLATFORM}
ccflags-y += -O2

pulp-objs := pulp_module.o pulp_mem.o pulp_rab.o pulp_dma.o pulp_mbox.o

ifeq ($(PLATFORM),5)
	# ZynqMP requires pulp_smmu.*
	pulp-objs += pulp_smmu.o
endif

pulp-c-objs := $(pulp-objs)

ifeq ($(KERNEL_ARCH),arm64)
	# JUNO and ZynqMP require the stripped down pulp_cache.S
	pulp-objs += pulp_cache.o
endif

.PHONY: all build deploy dis

all: deploy

build: modules

modules: $(patsubst %.o,%.c,$(pulp-c-objs)) $(patsubst %.o,%.h,$(pulp-c-objs))
	$(MAKE) -S ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(shell pwd) modules

deploy: build
	rsync -v pulp.ko $(HERO_TARGET_HOST):$(HERO_TARGET_PATH_DRIVER)/

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
