MAKEFLAGS=--warn-undefined-variables

NAME=uart

ARCH          := ${shell echo ${KERNEL_ARCH}}
CROSS_COMPILE := ${shell echo ${KERNEL_CROSS_COMPILE}}

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
OB=$(CROSS_COMPILE)objdump

FLAGS=-Wall -O2 -g

.PHONY: all build clean deploy dis

all: deploy

$(NAME): $(NAME).c
	$(CC) $(FLAGS) $< -o $@

build: $(NAME)

deploy: build
	rsync -v $(NAME) $(HERO_TARGET_HOST):$(HERO_TARGET_PATH)/

dis: build
	$(OB) -dS $(NAME).o > $(NAME).disasm

clean:
	-rm -f $(NAME)
