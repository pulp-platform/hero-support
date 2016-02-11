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

INCLUDES  = -I$(ARM_INC_DIR1) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2)
LIBS_STAT = -lsys_func
LIBDIRS   = -L$(shell echo $(ARM_LIB_DIR1))/lib

# Flags
FLAGS = -Wall -O2 -g

# For face_detect
CFLAGS := $(FLAGS) -I. $(INCLUDES) -DPULP
#CFLAGS := $(FLAGS) -I. $(INCLUDES)
EXE    := face_detect

CSRCS +=  cascade.c detect.c detect_core.c image.c integral.c list.c scaler.c sqrt64.c utils.c main.c
OBJS  = $(CSRCS:.c=.o)

all: 	lib $(EXE)
	echo all: make complete

lib:	$(C_FILES)
	$(foreach l,$(C_FILES),$(CC) -I$(ARM_INC_DIR1) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2) -c $(l) $(FLAGS);)
	$(AR) rvs $(ARM_LIB_PATH)libsys_func.a $(addsuffix .o,$(basename $(notdir $(C_FILES))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(C_FILES))))

$(EXE): $(OBJS)
	$(CC) $(LIBDIRS) -o $@ $^  $(LIBS_STAT)

############## Support routines
.PHONY : clean
clean:
	rm -f $(OBJS) $(EXE) *~ 
