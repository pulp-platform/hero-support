#ifndef PULP_FUNC_H__
#define PULP_FUNC_H__

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>   // for mmap
#include <fcntl.h>
#include <errno.h>      // for error codes
#include <stdbool.h>    // for bool
#include <sys/ioctl.h>  // for ioctl
#include <stdlib.h>     // for system
#include <unistd.h>     // for usleep, access

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
  unsigned int l3_offset; // used for pulp_l3_malloc
} PulpDev;

// shared variable data structure
typedef struct {
  void *ptr;
  size_t size;
  int type;
  unsigned char use_acp;
} DataDesc;

// task descriptor created by the compiler
typedef struct {
  int task_id; // used for RAB managment -> expiration date
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

int pulp_mailbox_read(PulpDev *pulp, unsigned *buffer, unsigned n_words);
void pulp_mailbox_clear_is(PulpDev *pulp);

int pulp_clking_set_freq(PulpDev *pulp, unsigned des_freq_mhz);
int pulp_clking_measure_freq(PulpDev *pulp);

void pulp_stdout_print(PulpDev *pulp, unsigned pe);
void pulp_stdout_clear(PulpDev *pulp, unsigned pe);

int pulp_rab_req(PulpDev *pulp, unsigned addr_start, unsigned size_b, 
		 unsigned char prot, unsigned char port,
                 unsigned char date_exp, unsigned char date_cur, unsigned char use_acp);
void pulp_rab_free(PulpDev *pulp, unsigned char date_cur);

int pulp_rab_req_striped(PulpDev *pulp, TaskDesc *task,
			 unsigned **data_idxs, int n_elements,  
                         unsigned char prot, unsigned char port, unsigned char use_acp);
void pulp_rab_free_striped(PulpDev *pulp);

void pulp_rab_mh_enable(PulpDev *pulp, unsigned char use_acp);
void pulp_rab_mh_disable(PulpDev *pulp);

int pulp_dma_xfer(PulpDev *pulp, 
		  unsigned addr_l3, unsigned addr_pulp, unsigned size_b,
		  unsigned host_read);

int pulp_omp_offload_task(PulpDev *pulp, TaskDesc *task);

void pulp_reset(PulpDev *pulp, unsigned full);
int  pulp_boot(PulpDev *pulp, TaskDesc *task);

int  pulp_load_bin(PulpDev *pulp, char *name);
void pulp_exe_start(PulpDev *pulp);
void pulp_exe_stop(PulpDev *pulp);
int  pulp_exe_wait(PulpDev *pulp, int timeout_s);

// required for ROD, CT, MJPEG

unsigned int pulp_l3_malloc(PulpDev *pulp, size_t size_b, unsigned *p_addr);
void         pulp_l3_free(PulpDev *pulp, unsigned v_addr, unsigned p_addr);

int pulp_offload_get_data_idxs(TaskDesc *task, unsigned **data_idxs);
int pulp_offload_rab_setup(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs, int n_idxs);
int pulp_offload_pass_desc(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs);
int pulp_offload_get_desc(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs, int n_idxs);

int pulp_offload_out(PulpDev *pulp, TaskDesc *task);
int pulp_offload_in(PulpDev *pulp, TaskDesc *task);

int pulp_offload_out_contiguous(PulpDev *pulp, TaskDesc *task, TaskDesc **ftask);
int pulp_offload_in_contiguous(PulpDev *pulp, TaskDesc *task, TaskDesc **ftask);

int pulp_offload_start(PulpDev *pulp, TaskDesc *task);
int pulp_offload_wait(PulpDev *pulp, TaskDesc *task);

// for random_forest
int pulp_rab_req_striped_mchan_img(PulpDev *pulp, unsigned char prot, unsigned char port,
				   unsigned p_height, unsigned p_width, unsigned i_step,
				   unsigned n_channels, unsigned char **channels,
				   unsigned *s_height);


#endif // PULP_FUNC_H__
