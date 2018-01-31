/* Copyright (C) 2017 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This code is under development and not yet released to the public.
 * Until it is released, the code is under the copyright of ETH Zurich and
 * the University of Bologna, and may contain confidential and/or unpublished 
 * work. Any reuse/redistribution is strictly forbidden without written
 * permission from ETH Zurich.
 *
 * Bug fixes and contributions will eventually be released under the
 * SolderPad open hardware license in the context of the PULP platform
 * (http://www.pulp-platform.org), under the copyright of ETH Zurich and the
 * University of Bologna.
 */
#ifndef _PULP_MODULE_H_
#define _PULP_MODULE_H_

#include <linux/cdev.h>   /* cdev struct */
#include <linux/device.h> /* device struct */

#include "pulp_host.h"    /* macros, struct definitions */

#if   PLATFORM == JUNO
  #include "juno.h"
#elif PLATFORM == TE0808
  #include "zynqmp.h"
#else
  #include "zynq.h"
#endif

#define DEBUG_LEVEL_PULP    0
#define DEBUG_LEVEL_OF      0
#define DEBUG_LEVEL_MEM     0
#define DEBUG_LEVEL_RAB     0
#define DEBUG_LEVEL_RAB_STR 0
#define DEBUG_LEVEL_RAB_MH  0
#define DEBUG_LEVEL_SMMU    0
#define DEBUG_LEVEL_SMMU_FH 0
#define DEBUG_LEVEL_DMA     0
#define DEBUG_LEVEL_MBOX    0

//#define PROFILE_DMA
//#define PROFILE_RAB_STR
//#define PROFILE_RAB_MH
//#define PROFILE_RAB_MH_SIMPLE

// macros
#if PLATFORM == JUNO || PLATFORM == TE0808
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
  #if PLATFORM == JUNO || PLATFORM == TE0808
    // device tree
    struct device * dt_dev_ptr;
    int intr_reg_irq;
  #endif
  // virtual address pointers for ioremap_nocache()
  void *mbox;
  void *rab_config;
  void *gpio;
  void *soc_periph;
  void *clusters;
  void *l3_mem;
  void *l2_mem;
  #if PLATFORM == TE0808
    void *smmu;
  #endif // PLATFORM
  #if PLATFORM == JUNO || PLATFORM == TE0808
    void *intr_reg;
  #endif // PLATFORM
  #if PLATFORM == ZEDBOARD || PLATFORM == ZC706 || PLATFORM == MINI_ITX
    void *slcr;
    void *mpcore;
  #endif // PLATFORM
  #if PLATFORM == ZEDBOARD || PLATFORM == ZC706 || PLATFORM == MINI_ITX || PLATFORM == TE0808
    void *uart;
  #endif // PLATFORM
  #if RAB_AX_LOG_EN == 1
    void *rab_ar_log;
    void *rab_aw_log;
    void *rab_cfg_log;
  #endif // RAB_AX_LOG_EN == 1
} PulpDev;

#endif // _PULP_MODULE_H_
