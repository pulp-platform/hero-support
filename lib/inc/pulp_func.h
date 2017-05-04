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

#include <errno.h>

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
  PulpSubDev mbox;
  PulpSubDev l2_mem;
  PulpSubDev l3_mem; 
  PulpSubDev gpio;
  PulpSubDev clking;
  PulpSubDev rab_config;
  PulpSubDev pulp_res_v_addr;
  PulpSubDev l3_mem_res_v_addr;
#if PLATFORM != JUNO
  PulpSubDev slcr;
  PulpSubDev mpcore;
#endif
  unsigned int l3_offset;      // used for pulp_l3_malloc
  unsigned int cluster_sel;    // cluster select
  unsigned int rab_ax_log_en;  // enable RAB AR/AW logger (Juno only)
} PulpDev;

// striping informationg structure
typedef struct {
  unsigned n_stripes;
  unsigned first_stripe_size_b;
  unsigned last_stripe_size_b;
  unsigned stripe_size_b;
} StripingDesc;

// shared variable data structure
typedef struct {
  void         * ptr;         // address in host virtual memory
  void         * ptr_l3_v;    // host virtual address in contiguous L3 memory   - filled by runtime library based on sh_mem_ctrl
  void         * ptr_l3_p;    // PULP physical address in contiguous L3 memory  - filled by runtime library based on sh_mem_ctrl
  size_t         size;        // size in Bytes
  ElemType       type;
  unsigned char  sh_mem_ctrl; // 0: no SVM, copy-based sharing using contiguous L3 memory
                              // 1: SVM, set up mapping at offload time, might fail - use with caution
                              // 2: SVM, use striping (L1 only), might fail - use with caution
                              // 3: SVM, use miss handling
                              // 4: no SVM, copy-based sharing using contiguous L3 memory, but let the accelerator do the tryx()
  unsigned char  cache_ctrl;  // 0: flush caches, access through DRAM
                              // 1: do not flush caches, access through caches
  unsigned char  rab_lvl;     // 0: first L1, L2 when full
                              // 1: L1 only
                              // 2: L2 only
  StripingDesc * stripe_desc; // only used if sh_mem_ctrl = 2
} DataDesc;

// task descriptor created by the compiler
typedef struct {
  int        task_id; // used for RAB managment -> expiration date
  char     * name;
  int        n_clusters;
  int        n_data;
  DataDesc * data_desc;
} TaskDesc;

// function prototypes
int  pulp_reserve_v_addr(PulpDev *pulp);
int  pulp_free_v_addr(const PulpDev *pulp);
void pulp_print_v_addr(PulpDev *pulp);

int  pulp_read32(const unsigned *base_addr, unsigned off, char off_type);
void pulp_write32(unsigned *base_addr, unsigned off, char off_type, unsigned value);

int pulp_mmap(PulpDev *pulp);
int pulp_munmap(PulpDev *pulp);
int pulp_init(PulpDev *pulp);

int pulp_mbox_read(const PulpDev *pulp, unsigned *buffer, unsigned n_words);
int pulp_mbox_write(PulpDev *pulp, unsigned word);
void pulp_mbox_clear_is(PulpDev *pulp);

int pulp_clking_set_freq(PulpDev *pulp, unsigned des_freq_mhz);
int pulp_clking_measure_freq(PulpDev *pulp);

int pulp_rab_req(const PulpDev *pulp, unsigned addr_start, unsigned size_b,
                 unsigned char prot, unsigned char port,
                 unsigned char date_exp, unsigned char date_cur,
                 unsigned char use_acp, unsigned char rab_lvl);
void pulp_rab_free(const PulpDev *pulp, unsigned char date_cur);

int pulp_rab_req_striped(const PulpDev *pulp, const TaskDesc *task,
                         unsigned **data_idxs, int n_elements);
void pulp_rab_free_striped(const PulpDev *pulp);

int  pulp_rab_mh_enable(const PulpDev *pulp, unsigned char use_acp, unsigned char rab_mh_lvl);
void pulp_rab_mh_disable(const PulpDev *pulp);

int pulp_rab_soc_mh_enable(const PulpDev* pulp, const unsigned static_2nd_lvl_slices);
int pulp_rab_soc_mh_disable(const PulpDev* pulp);

int pulp_rab_ax_log_read(const PulpDev* pulp);

int pulp_dma_xfer(const PulpDev *pulp,
                  unsigned addr_l3, unsigned addr_pulp, unsigned size_b,
                  unsigned host_read);

int pulp_omp_offload_task(PulpDev *pulp, TaskDesc *task);

void pulp_reset(PulpDev *pulp, unsigned full);
int  pulp_boot(PulpDev *pulp, const TaskDesc *task);

int  pulp_load_bin(PulpDev *pulp, const char *name);
void pulp_exe_start(PulpDev *pulp);
void pulp_exe_stop(PulpDev *pulp);
int  pulp_exe_wait(const PulpDev *pulp, int timeout_s);

unsigned int pulp_l3_malloc(PulpDev *pulp, size_t size_b, unsigned *p_addr);
void         pulp_l3_free(PulpDev *pulp, unsigned v_addr, unsigned p_addr);

int pulp_offload_get_data_idxs(const TaskDesc *task, unsigned **data_idxs);
int pulp_offload_rab_setup(const PulpDev *pulp, const TaskDesc *task, unsigned **data_idxs, int n_idxs);
int pulp_offload_l3_copy_raw_out(PulpDev *pulp, TaskDesc *task, const unsigned **data_idxs);
int pulp_offload_l3_copy_raw_in(PulpDev *pulp, const TaskDesc *task, const unsigned **data_idxs);
int pulp_offload_pass_desc(PulpDev *pulp, const TaskDesc *task, const unsigned **data_idxs);
int pulp_offload_get_desc(const PulpDev *pulp, TaskDesc *task, const unsigned **data_idxs);

int pulp_offload_out(PulpDev *pulp, TaskDesc *task);
int pulp_offload_in(PulpDev *pulp, TaskDesc *task);

// required for ROD, CT, MJPEG
int pulp_offload_out_contiguous(PulpDev *pulp, TaskDesc *task, TaskDesc **ftask);
int pulp_offload_in_contiguous(PulpDev *pulp, TaskDesc *task, TaskDesc **ftask);

int pulp_offload_start(PulpDev *pulp, const TaskDesc *task);
int pulp_offload_wait(const PulpDev *pulp, const TaskDesc *task);

// for random_forest
int pulp_rab_req_striped_mchan_img(const PulpDev *pulp, unsigned char prot, unsigned char port,
                                   unsigned p_height, unsigned p_width, unsigned i_step,
                                   unsigned n_channels, unsigned char **channels,
                                   unsigned *s_height);


#endif // PULP_FUNC_H__
