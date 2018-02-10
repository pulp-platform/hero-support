/*
 * This file is part of the PULP device driver.
 *
 * Copyright (C) 2018 ETH Zurich, University of Bologna
 *
 * Author: Pirmin Vogel <vogelpi@iis.ee.ethz.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    void *cci;
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
