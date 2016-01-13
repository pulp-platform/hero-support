# Name of the program
NAME=force_cache_miss_rate

# Cross compilation stuff
ARCH=arm
CROSS_COMPILE=arm-xilinx-linux-gnueabi-

# Paths to libraries and header files
ARM_INC_DIR=$(shell echo $(ARM_INC_DIR1))
PULP_INC_DIR=$(shell echo $(PULP_INC_DIR1))
PULP_INC_DIR2 :=$(shell echo $(PULP_INC_DIR2))

LIB_PATH=$(shell echo $(ARM_LIB_DIR1))/lib/
C_PATH=$(shell echo $(ARM_LIB_DIR1))/src/
C_FILES=$(C_PATH)pulp_func.c $(C_PATH)arm_func.c $(C_PATH)zynq_pmm_user.c

C_FILES2=ldr_64_lines.c ldr_64x8_words.c ldmia8_64_lines.c ldrd_64_lines.c str_64_lines.c str_64x8_words.c stmia8_64_lines.c

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

#FLAGS=-Wall -O2 -g
FLAGS=-Wall -O0 -g


lib:	$(C_FILES)
	$(foreach l,$(C_FILES),$(CC) -I$(ARM_INC_DIR) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2) -c $(l) $(FLAGS);)
	$(AR) rvs $(LIB_PATH)libsys_func.a $(addsuffix .o,$(basename $(notdir $(C_FILES))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(C_FILES))))

all: 	lib
	$(CC) $(FLAGS) $(NAME).c -static -I$(ARM_INC_DIR) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2) -L$(LIB_PATH) -lsys_func -lm -o $(NAME)

read:	lib $(C_FILES2) 	
	$(CC) $(FLAGS) $(NAME).c -static -I$(ARM_INC_DIR) -I$(PULP_INC_DIR1) -I$(PULP_INC_DIR2) -L$(LIB_PATH) -lsys_func -lm -c $(NAME)
	$(OB) -dS $(NAME).o > $(NAME).read
