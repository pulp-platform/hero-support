# Copyright (C) 2018 ETH Zurich and University of Bologna
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
MAKEFLAGS=--warn-undefined-variables

CC       := ${CROSS_COMPILE}gcc
AR       := ${CROSS_COMPILE}ar
OBJCDUMP := ${CROSS_COMPILE}objdump

SRC_DIR  := src
INC_DIR  := inc
LIB_DIR  := lib

SOURCES  := $(SRC_DIR)/pulp.c
CFLAGS   := -Wall -O3 -g -DPLATFORM=${PLATFORM}
CFLAGS_SO:= $(CFLAGS) -shared -fPIC

SO       := $(LIB_DIR)/libpulp.so
A        := $(LIB_DIR)/libpulp.a

.PHONY: all clean deploy

all: $(SO) $(A) deploy

$(SO): $(SOURCES)
	mkdir -p $(LIB_DIR)
	$(foreach l,$(SOURCES),$(CC) -I${INC_DIR} -I${HERO_PULP_INC_DIR} -c $(l) $(CFLAGS_SO);)
	$(CC) $(CFLAGS_SO) -o $@ $(addsuffix .o,$(basename $(notdir $(SOURCES))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(SOURCES))))

$(A): $(SOURCES)
	mkdir -p $(LIB_DIR)
	$(foreach l,$(SOURCES),$(CC) -I${INC_DIR} -I${HERO_PULP_INC_DIR} -c $(l) $(CFLAGS);)
	$(AR) rvs -o $@ $(addsuffix .o,$(basename $(notdir $(SOURCES))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(SOURCES))))

clean:
	-rm -rf $(LIB_DIR) $(SO) $(A)
	-rm -rf $(addsuffix .o,$(basename $(notdir $(SOURCES))))

deploy:
	rsync -rv $(LIB_DIR)/* ${HERO_TARGET_HOST}:${HERO_TARGET_PATH_LIB}
