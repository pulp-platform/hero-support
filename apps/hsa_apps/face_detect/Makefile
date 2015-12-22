# Cross compilation stuff
ARCH=arm
CROSS_COMPILE=arm-xilinx-linux-gnueabi-

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

# For PULP
# Paths to libraries and header files
ARM_INC_DIR1  := $(shell echo $(ARM_INC_DIR1))
PULP_INC_DIR1 := $(shell echo $(PULP_INC_DIR1))
PULP_INC_DIR2 := $(shell echo $(PULP_INC_DIR2))

ARM_LIB_PATH=$(shell echo $(ARM_LIB_DIR1))/lib/
C_PATH=$(shell echo $(ARM_LIB_DIR1))/src/
C_FILES=$(C_PATH)pulp_func.c $(C_PATH)arm_func.c $(C_PATH)zynq_pmm_user.c

# For face_detect
CFLAGS := -O2 -Wall -g -I.
EXE    := face_detect

CSRCS +=  cascade.c detect.c detect_core.c image.c integral.c list.c scaler.c sqrt64.c utils.c main.c
OBJS  = $(CSRCS:.c=.o)

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) -o $@ $^

############## Support routines
.PHONY : clean
clean:
	rm -f $(OBJS) $(EXE) *~ 
