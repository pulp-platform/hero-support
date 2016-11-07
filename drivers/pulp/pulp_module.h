#ifndef _PULP_MODULE_H_
#define _PULP_MODULE_H_

#include <linux/cdev.h>		/* cdev struct */

#include "pulp_host.h"    /* macros, struct definitions */

#if PLATFORM == JUNO
  #include "juno.h"
#else
  #include "zynq.h"
#endif

#define DEBUG_LEVEL_PULP    0
#define DEBUG_LEVEL_MEM     0
#define DEBUG_LEVEL_RAB     0
#define DEBUG_LEVEL_RAB_STR 0
#define DEBUG_LEVEL_RAB_MH  0
#define DEBUG_LEVEL_DMA     0
#define DEBUG_LEVEL_MBOX    0
//#define PROFILE_DMA
//#define PROFILE_RAB_STR
//#define PROFILE_RAB_MH

// macros
#if PLATFORM == JUNO
  #define IOWRITE_L(value, addr) ( iowrite64(value, addr) )
  #define IOREAD_L(addr)         ( ioread64(addr)         )
#else
  #define IOWRITE_L(value, addr) ( iowrite32(value, addr) )
  #define IOREAD_L(addr)         ( ioread32(addr)         )
#endif

// type definitions
typedef struct {
  dev_t dev; // device number
  struct file_operations *fops;
  struct cdev cdev;
  int minor;
  int major;
  void *mbox;
  void *rab_config;
  void *gpio;
  void *soc_periph;
  void *clusters;
  void *l3_mem;
  void *l2_mem;
  #if PLATFORM == JUNO
    void *intr_reg;
    void *rab_ar_log;
    void *rab_aw_log;
  #else
    void *slcr;
    void *mpcore;
    void *uart0;
  #endif // PLATFORM == JUNO
} PulpDev;

// NEW - move to pulp_rab.h/c when cleaning up ioctl stuff
void pulp_rab_update(unsigned update_req);
void pulp_rab_switch(void);

#if PLATFORM == JUNO
  void pulp_rab_ax_log_read(unsigned pause);
  void pulp_rab_ax_log_print(void);
#endif

#endif // _PULP_MODULE_H_ 
