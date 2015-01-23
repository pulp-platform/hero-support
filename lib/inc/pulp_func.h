#ifndef PULP_FUNC_H__
#define PULP_FUNC_H__

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>   // for mmap
#include <fcntl.h>
#include <errno.h>      // for error codes
#include <stdbool.h>    // for bool
#include <unistd.h>     // for sleep
#include <sys/ioctl.h>  // for ioctl
#include <stdlib.h>     // for system


#include "zynq.h"
#include "pulp_host.h"

// type definitions
typedef struct {
  unsigned *v_addr;
  size_t size;
} PulpSubDev;

typedef struct {
  int fd; // file descriptor
  PulpSubDev clusters;
  PulpSubDev soc_periph;
  PulpSubDev mailbox;
  PulpSubDev l2_mem;
  PulpSubDev l3_mem; 
  PulpSubDev gpio;
  PulpSubDev clking;
  PulpSubDev stdout;
  PulpSubDev rab_config;
  PulpSubDev slcr;
  PulpSubDev mpcore;
  PulpSubDev reserved_v_addr;
} PulpDev;

// shared variable data structure
typedef struct {
  unsigned *v_addr;
  size_t size;
} DataDesc;

// task descriptor created by the compiler
typedef struct {
  char *name;
  int n_clusters;
  int n_data;
  DataDesc *data_desc;
} TaskDesc;

// function prototypes
int  pulp_reserve_v_addr(PulpDev *pulp);
int  pulp_free_v_addr(PulpDev *pulp);
void pulp_print_v_addr(PulpDev *pulp);

int  pulp_read32(unsigned *base_addr, unsigned off, char off_type);
void pulp_write32(unsigned *base_addr, unsigned off, char off_type, unsigned value);

int pulp_mmap(PulpDev *pulp);
int pulp_munmap(PulpDev *pulp);
int pulp_init(PulpDev *pulp);

int  pulp_rab_req(PulpDev *pulp, unsigned addr_start, unsigned size_b, 
		  unsigned char prot, unsigned char port, unsigned char date_exp, unsigned char date_cur);
void pulp_rab_free(PulpDev *pulp, unsigned char date_cur);

int pulp_dma_xfer(PulpDev *pulp, unsigned addr_l3, unsigned addr_pulp, unsigned size_b, unsigned host_read);

int pulp_omp_offload_task(PulpDev *pulp, TaskDesc *task);

int pulp_check_results(PulpDev *pulp);

void pulp_reset(PulpDev *pulp);

#endif // PULP_FUNC_H__
