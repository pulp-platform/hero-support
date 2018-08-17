# Name of the program
NAME=uart

# Cross compilation stuff
PLATFORM      := ${shell echo ${PLATFORM}}

ARCH          := ${shell echo ${KERNEL_ARCH}}
CROSS_COMPILE := ${shell echo ${KERNEL_CROSS_COMPILE}}

# HERO target
HERO_TARGET_HOST := ${shell echo ${HERO_TARGET_HOST}}
HERO_TARGET_PATH := ${shell echo ${HERO_TARGET_PATH}}

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

FLAGS=-Wall -O2 -g

.PHONY: all build deploy dis

all: deploy

$(NAME): $(NAME).c
	$(CC) $(FLAGS) $< -o $@

build: $(NAME)

deploy: build
	rsync -v $(NAME) $(HERO_TARGET_HOST):$(HERO_TARGET_PATH)/

dis: build
	$(OB) -dS $(NAME).o > $(NAME).disasm
