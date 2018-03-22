#! /bin/bash
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

CC       := ${CROSS_COMPILE}gcc

SRCS_DIR := src
INCS_DIR := inc
LIB      := lib

SRCS     := $(SRCS_DIR)/pulp_func.c $(SRCS_DIR)/zynq_pmm_user.c
CFLAGS   := -Wall -O3 -g2 -shared -fPIC -DPLATFORM=${PLATFORM}

SO       := $(LIB)/libpulp-offload.so

$(SO): check-env $(SRCS)
	mkdir -p $(LIB)
	$(foreach l,$(SRCS),$(CC) -I${INCS_DIR} -I${PULP_INC_DIR1} -I${PULP_INC_DIR2} -c $(l) $(CFLAGS);)
	$(CC) $(CFLAGS) -o $@ $(addsuffix .o,$(basename $(notdir $(SRCS))))
	-rm -rf $(addsuffix .o,$(basename $(notdir $(SRCS))))

all: $(SO)

.PHONY: clean check-env
clean:
	-rm -rf $(LIB) $(SO)
	-rm -rf $(addsuffix .o,$(basename $(notdir $(SRCS))))


check-env:
ifndef PLATFORM
  $(error PLATFORM is undefined)
endif
ifndef CROSS_COMPILE
  $(error CROSS_COMPILE is undefined)
endif
ifndef PULP_INC_DIR1
  $(error PULP_INC_DIR1 is undefined)
endif
ifndef PULP_INC_DIR2
  $(error PULP_INC_DIR2 is undefined)
endif
