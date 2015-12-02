# Cross compilation stuff
ARCH=arm
CROSS_COMPILE=arm-xilinx-linux-gnueabi-

CC=$(CROSS_COMPILE)g++
CCC=$(CROSS_COMPILE)gcc
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

# For CRForest-Detector
INCLUDES = 	-I$(shell echo $(ARM_LIB_EXT_DIR))/include/opencv \
		-I$(shell echo $(ARM_LIB_EXT_DIR))/include -I$(ARM_INC_DIR1) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2)

LIBS_DYN = 	-Wl,-Bdynamic -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio \
		-lavcodec -lavformat -lavutil -lswscale -lbz2 -lswresample \
		-lopencv_video -lopencv_highgui -lopencv_ml

LIBS_STAT = 	-Wl,-Bstatic -lsys_func -lm -Wl,-Bdynamic

LIBDIRS = 	-L$(shell echo $(ARM_LIB_EXT_DIR))/lib \
		-L$(shell echo $(WORKSPACE_DIR))/buildroot/output/target/usr/lib \
		-L$(shell echo $(ARM_LIB_DIR1))/lib \
		-L/usr/pack/vivado-2015.1-kgf/SDK/2015.1/gnu/arm/lin/arm-xilinx-linux-gnueabi/libc/lib

# Flags
OPT = -O3 -Wno-deprecated -DPULP
FLAGS = -Wall -O2 -g

.PHONY: lib all clean

OBJS = CRForest-Detector.o CRPatch.o HoG.o CRForestDetector.o CRTree.o

clean:	
	rm -f *.o *~ CRForest-Detector

all:	lib CRForest-Detector
	echo all: make complete

%.o:	%.cpp
	$(CC) -c $(INCLUDES) $+ $(OPT)

CRForest-Detector: 	$(OBJS)
			$(CC) $(LIBDIRS) -o $@ $+ $(OPT) $(LIBS_DYN) $(LIBS_STAT)

lib:	$(C_FILES)
	$(foreach l,$(C_FILES),$(CCC) -I$(ARM_INC_DIR1) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2) -c $(l) $(FLAGS);)
	$(AR) rvs $(ARM_LIB_PATH)libsys_func.a $(addsuffix .o,$(basename $(notdir $(C_FILES))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(C_FILES))))


