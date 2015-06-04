#ifndef _PULP_MODULE_H_
#define _PULP_MODULE_H_

#include <linux/cdev.h>		/* cdev struct */

#define DEBUG_LEVEL_PULP 0
#define DEBUG_LEVEL_MEM  0
#define DEBUG_LEVEL_RAB  0
#define DEBUG_LEVEL_DMA  0
#define DEBUG_LEVEL_MBOX 0
//#define PROFILE_DMA
//#define PROFILE_RAB

// type definitions
typedef struct {
  dev_t dev; // device number
  struct file_operations *fops;
  struct cdev cdev;
  int minor;
  int major;
  void *l3_mem;
  void *l2_mem;
  void *mailbox;
  void *soc_periph;
  void *clusters;
  void *rab_config;
  void *gpio;
  void *slcr;
  void *mpcore;
} PulpDev;

#endif // _PULP_MODULE_H_ 
