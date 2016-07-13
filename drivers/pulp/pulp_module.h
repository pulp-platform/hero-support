#ifndef _PULP_MODULE_H_
#define _PULP_MODULE_H_

#include <linux/cdev.h>		/* cdev struct */

#include "pulp_host.h"    /* for JUNO */

#define DEBUG_LEVEL_PULP   0
#define DEBUG_LEVEL_MEM    0
#define DEBUG_LEVEL_RAB    0
#define DEBUG_LEVEL_RAB_MH 0
#define DEBUG_LEVEL_DMA    0
#define DEBUG_LEVEL_MBOX   0
//#define PROFILE_DMA
//#define PROFILE_RAB
//#define PROFILE_RAB_MH

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
  #else
    void *slcr;
    void *mpcore;
    void *uart0;
  #endif // PLATFORM == JUNO
} PulpDev;

// NEW - move to pulp_rab.h/c when cleaning up ioctl stuff
void pulp_rab_update(void);
void pulp_rab_switch(void);

#endif // _PULP_MODULE_H_ 
