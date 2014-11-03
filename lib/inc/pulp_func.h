#ifndef PULP_FUNC_H__
#define PULP_FUNC_H__

#include <stdio.h>
#include <string.h>
#include <sys/mman.h> // for mmap
#include <fcntl.h>
#include <errno.h> // for error codes
#include <stdbool.h> // for bool
#include <unistd.h> // for sleep

#include "zynq.h"
#include "pulp_host.h"

// type definitions
typedef struct {
  unsigned int *v_addr;
  size_t size;
} PulpSubDev;

typedef struct {
  int fd; // file descriptor
  PulpSubDev l3_mem; 
  PulpSubDev clusters;
  PulpSubDev soc_periph;
  PulpSubDev l2_mem;
  PulpSubDev rab_config;
  PulpSubDev gpio;
  PulpSubDev slcr;
  PulpSubDev reserved_v_addr;
} PulpDev;

// function prototypes
int pulp_reserve_v_addr(PulpDev *pulp);
int pulp_free_v_addr(PulpDev *pulp);

int pulp_read32(unsigned *base_addr, unsigned off, char off_type);
void pulp_write32(unsigned *base_addr, unsigned off, char off_type, unsigned value);

int pulp_dma_transfer(PulpDev *pulp, unsigned source_addr, unsigned dest_addr, unsigned length_b, unsigned dma);
int pulp_dma_wait(PulpDev *pulp, unsigned dma);
int pulp_dma_reset(PulpDev *pulp, unsigned dma);

int pulp_mmap(PulpDev *pulp);
int pulp_munmap(PulpDev *pulp);
int pulp_init(PulpDev *pulp);

int pulp_offload_kernel(PulpDev *pulp, bool sync);

int pulp_check_results(PulpDev *pulp);

void pulp_reset(PulpDev *pulp);

#endif // PULP_FUNC_H__
