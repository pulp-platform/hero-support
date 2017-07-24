# Name of the program
NAME=smmu_if_user

# Cross compilation stuff
PLATFORM      := ${shell echo ${PLATFORM}}

ARCH          := ${shell echo ${ARCH}}
CROSS_COMPILE := ${shell echo ${CROSS_COMPILE}}

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

FLAGS=-Wall -O2 -g -DPLATFORM=${PLATFORM}	

all:
	$(CC) $(FLAGS) $(NAME).c -static -lm -o $(NAME)

read:
	$(CC) $(FLAGS) $(NAME).c -static -lm -c $(NAME)
	$(OB) -dS $(NAME).o > $(NAME).read
