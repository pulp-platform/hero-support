# Name of the program
NAME=calibrator

# Cross compilation stuff
ARCH=arm
CROSS_COMPILE=arm-xilinx-linux-gnueabi-

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

FLAGS=-Wall -O2 -g 	

all:	${NAME}.c
	$(CC) $(FLAGS) $(NAME).c -static -lm -o $(NAME)
