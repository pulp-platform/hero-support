# Name of the program
NAME=testing

# Cross compilation stuff
ARCH=arm
CROSS_COMPILE=arm-xilinx-linux-gnueabi-

# Paths to libraries and header files
LIB_PATH=../../lib/lib/
H_PATH=../../lib/inc/
C_PATH=../../lib/src/
C_FILES=$(C_PATH)pulp_func.c $(C_PATH)arm_func.c $(C_PATH)zynq_pmm_user.c

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

FLAGS=-Wall -O2 -g 	

lib:	$(C_FILES)
	$(foreach l,$(C_FILES),$(CC) -I$(H_PATH) -c $(l) $(FLAGS);)
	$(AR) rvs $(LIB_PATH)libsys_func.a $(addsuffix .o,$(basename $(notdir $(C_FILES))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(C_FILES))))

all: 	lib
	$(CC) $(FLAGS) $(NAME).c -static -I$(H_PATH) -L$(LIB_PATH) -lsys_func -lm -o $(NAME)

read:	lib
	$(CC) $(FLAGS) $(NAME).c -static -I$(H_PATH) -L$(LIB_PATH) -lsys_func -lm -c $(NAME)
	$(OB) -dS $(NAME).o > $(NAME).read
