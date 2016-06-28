# Name of the program
NAME=uart

# Cross compilation stuff
PLATFORM      := ${shell echo ${PLATFORM}}

ARCH          := ${shell echo ${ARCH}}
CROSS_COMPILE := ${shell echo ${CROSS_COMPILE}}

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

FLAGS=-Wall -O2 -g

all:
	$(CC) $(FLAGS) $(NAME).c -o $(NAME)

read:	all
	$(CC) $(FLAGS) $(NAME).c -c $(NAME)
	$(OB) -dS $(NAME).o > $(NAME).read
