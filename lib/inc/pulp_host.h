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
#ifndef PULP_HOST_H___
#define PULP_HOST_H___

// PLATFORM is exported in sourceme.sh and passed by the Makefile
#define ZEDBOARD 1
#define ZC706    2
#define MINI_ITX 3
#define JUNO     4

// from include/archi/pulp.h
#define CHIP_BIGPULP         7
#define CHIP_BIGPULP_Z_7020  8
#define CHIP_BIGPULP_Z_7045  9

#define PULP_CHIP_FAMILY CHIP_BIGPULP
#if PLATFORM == ZEDBOARD
  #define PULP_CHIP CHIP_BIGPULP_Z_7020
#elif PLATFORM == ZC706 || PLATFORM == MINI_ITX
  #define PULP_CHIP CHIP_BIGPULP_Z_7045
#else // PLATFORM == JUNO
  #define PULP_CHIP CHIP_BIGPULP
#endif

#include "archi/bigpulp/pulp.h"

#define DEBUG_LEVEL 0

// mailbox communication
#define PULP_READY 0x1
#define PULP_START 0x2
#define PULP_BUSY  0x3
#define PULP_DONE  0x4
#define PULP_STOP  0xF

#define HOST_READY 0x1000
#define HOST_DONE  0x3000

#define MBOX_N_BITS_REQ_TYPE   4  // number of MSBs to specify the type
#define RAB_UPDATE_N_BITS_ELEM 8  // number of bits to specify the mask of elements to be updated
#define RAB_UPDATE_N_BITS_TYPE 2  // number of bits to specify the update type

#define TO_RUNTIME 0x10000000 // bypass PULP driver
#define RAB_UPDATE 0x20000000 // handled by PULP driver
#define RAB_SWITCH 0x30000000 // handled by PULP driver

/*
 * Macros
 */
#define BIT_N_SET(n)        ( 1UL << (n) )
#define BIT_GET(y, mask)    ( y & mask )
#define BIT_SET(y, mask)    ( y |=  (mask) )
#define BIT_CLEAR(y, mask)  ( y &= ~(mask) )
#define BIT_FLIP(y, mask)   ( y ^=  (mask) )
// Create a bitmask of length len.
#define BIT_MASK_GEN(len)       ( BIT_N_SET(len)-1 )
// Create a bitfield mask of length len starting at bit start.
#define BF_MASK_GEN(start, len)      ( BIT_MASK_GEN(len) << (start) )
// Prepare a bitmask for insertion or combining.
#define BF_PREP(x, start, len)   ( ((x)&BIT_MASK_GEN(len)) << (start) )
// Extract a bitfield of length len starting at bit start from y.
#define BF_GET(y, start, len)    ( ((y)>>(start)) & BIT_MASK_GEN(len) )
// Insert a new bitfield value x into y.
#define BF_SET(y, x, start, len) \
  ( y= ((y) &~ BF_MASK_GEN(start, len)) | BF_PREP(x, start, len) )

// RAB related
#define RAB_GET_FLAGS_HW(flags_hw, request) \
  ( flags_hw = BF_GET(request, 0, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_ACP) )
#define RAB_SET_FLAGS_HW(request, flags_hw) \
  ( BF_SET(request, flags_hw, 0, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_ACP) )

#define RAB_GET_PROT(prot, request) ( prot = request & 0x7 )
#define RAB_SET_PROT(request, prot) \
  ( BF_SET(request, prot, 0, RAB_CONFIG_N_BITS_PROT) )

#define RAB_GET_ACP(use_acp, request) \
  ( use_acp = BF_GET(request, RAB_CONFIG_N_BITS_PROT, RAB_CONFIG_N_BITS_ACP) )
#define RAB_SET_ACP(request, use_acp) \
  ( BF_SET(request, use_acp, RAB_CONFIG_N_BITS_PROT, RAB_CONFIG_N_BITS_ACP) )

#define RAB_GET_PORT(port, request) \
  ( port = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_ACP, RAB_CONFIG_N_BITS_PORT) )
#define RAB_SET_PORT(request, port) \
  ( BF_SET(request, port, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_ACP, RAB_CONFIG_N_BITS_PORT) )

#define RAB_GET_LVL(rab_lvl, request) \
  ( rab_lvl = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		       + RAB_CONFIG_N_BITS_ACP, RAB_CONFIG_N_BITS_LVL) )
#define RAB_SET_LVL(request, rab_lvl) \
  ( BF_SET(request, rab_lvl, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	         + RAB_CONFIG_N_BITS_ACP, RAB_CONFIG_N_BITS_LVL) )

#define RAB_GET_DATE_EXP(date_exp, request) \
  ( date_exp = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
           + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_LVL, RAB_CONFIG_N_BITS_DATE) )
#define RAB_SET_DATE_EXP(request, date_exp) \
  ( BF_SET(request, date_exp, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	         + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_LVL, RAB_CONFIG_N_BITS_DATE) )

#define RAB_GET_DATE_CUR(date_cur, request) \
  ( date_cur = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		       + RAB_CONFIG_N_BITS_DATE + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_LVL, RAB_CONFIG_N_BITS_DATE) )
#define RAB_SET_DATE_CUR(request, date_cur) \
  ( BF_SET(request, date_cur, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	         + RAB_CONFIG_N_BITS_DATE + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_LVL, RAB_CONFIG_N_BITS_DATE) )

// for RAB striping - ATTENTION: not compatible with date_exp, date_cur!!!
#define RAB_GET_OFFLOAD_ID(offload_id, request) \
  ( offload_id = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
          + RAB_CONFIG_N_BITS_ACP, RAB_CONFIG_N_BITS_OFFLOAD_ID) )
#define RAB_SET_OFFLOAD_ID(request, offload_id) \
  ( BF_SET(request, offload_id, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
          + RAB_CONFIG_N_BITS_ACP, RAB_CONFIG_N_BITS_OFFLOAD_ID) )

#define RAB_GET_N_ELEM(n_elem, request) \
  ( n_elem = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		     + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID, RAB_CONFIG_N_BITS_N_ELEM) )
#define RAB_SET_N_ELEM(request, n_elem) \
  ( BF_SET(request, n_elem, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	       + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID, RAB_CONFIG_N_BITS_N_ELEM) )

#define RAB_GET_N_STRIPES(n_stripes, request) \
  ( n_stripes = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		     + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID + RAB_CONFIG_N_BITS_N_ELEM, \
		     RAB_CONFIG_N_BITS_N_STRIPES) )
#define RAB_SET_N_STRIPES(request, n_stripes) \
  ( BF_SET(request, n_stripes, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	       + RAB_CONFIG_N_BITS_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID + RAB_CONFIG_N_BITS_N_ELEM, \
	       RAB_CONFIG_N_BITS_N_STRIPES) )

#define MBOX_GET_REQ_TYPE(type, request) \
  ( type = BF_GET(request, 32-MBOX_N_BITS_REQ_TYPE, \
         MBOX_N_BITS_REQ_TYPE) << (32-MBOX_N_BITS_REQ_TYPE) )
#define MBOX_GET_N_WORDS(n_words, request) \
  ( n_words = BF_GET(request, 0, 32-MBOX_N_BITS_REQ_TYPE) )

#define RAB_UPDATE_GET_ELEM(elem_mask, request) \
  ( elem_mask = BF_GET(request, 0, RAB_UPDATE_N_BITS_ELEM) )
#define RAB_UPDATE_GET_TYPE(type, request) \
  ( type = BF_GET(request, RAB_UPDATE_N_BITS_ELEM, RAB_UPDATE_N_BITS_TYPE) )

// #define MAX(x, y) (((x) > (y)) ? (x) : (y))
// #define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*
 * General settings
 */
#define PULP_N_DEV_NUMBERS 1

/**
 * IOCTL Setup
 *
 * When defining a new IOCTL command, append a macro definition to the list below, using the
 * consecutively following command number, and increase the `PULP_IOC_NR_MAX` macro.
 */
#define PULP_IOCTL_MAGIC 'p'
#define PULP_IOC_NR_MIN 0xB0
#define PULP_IOC_NR_MAX 0xBA

#define PULP_IOCTL_RAB_REQ   _IOW(PULP_IOCTL_MAGIC,0xB0,unsigned) // ptr
#define PULP_IOCTL_RAB_FREE  _IOW(PULP_IOCTL_MAGIC,0xB1,unsigned) // value

#define PULP_IOCTL_RAB_REQ_STRIPED  _IOW(PULP_IOCTL_MAGIC,0xB2,unsigned) // ptr
#define PULP_IOCTL_RAB_FREE_STRIPED _IOW(PULP_IOCTL_MAGIC,0xB3,unsigned) // value

#define PULP_IOCTL_RAB_MH_ENA  _IOW(PULP_IOCTL_MAGIC,0xB4,unsigned) // value
#define PULP_IOCTL_RAB_MH_DIS  _IO(PULP_IOCTL_MAGIC,0xB5)

#define PULP_IOCTL_DMAC_XFER _IOW(PULP_IOCTL_MAGIC,0xB6,unsigned) // ptr

#define PULP_IOCTL_INFO_PASS _IOW(PULP_IOCTL_MAGIC,0xB7,unsigned) // ptr

#define PULP_IOCTL_RAB_SOC_MH_ENA _IOW(PULP_IOCTL_MAGIC,0xB8,unsigned) // value
#define PULP_IOCTL_RAB_SOC_MH_DIS _IO(PULP_IOCTL_MAGIC,0xB9)

#define PULP_IOCTL_RAB_AX_LOG_READ _IOW(PULP_IOCTL_MAGIC,0xBA,unsigned) // ptr

/*
 * Independent parameters
 */
#define H_GPIO_SIZE_B 0x1000

#define CLKING_SIZE_B                 0x1000
#define CLKING_CONFIG_REG_0_OFFSET_B  0x200
#define CLKING_CONFIG_REG_2_OFFSET_B  0x208
#define CLKING_CONFIG_REG_5_OFFSET_B  0x214
#define CLKING_CONFIG_REG_8_OFFSET_B  0x220
#define CLKING_CONFIG_REG_23_OFFSET_B 0x25C
#define CLKING_STATUS_REG_OFFSET_B    0x4

#define L3_MEM_BASE_ADDR     0x80000000 // Address at which PULP sees the contiguous L3

#define PGD_BASE_ADDR        0x20000000 // Address at which PULP sees the top-level page table of
                                        // the host process

#define RAB_CONFIG_SIZE_B         0x10000
#define RAB_L1_N_MAPPINGS_PORT_1  2
#define RAB_N_PORTS               2

#define RAB_SLICE_SIZE_B               0x20
#define RAB_SLICE_BASE_OFFSET_B        0x20
#define RAB_SLICE_ADDR_START_OFFSET_B  0x0
#define RAB_SLICE_ADDR_END_OFFSET_B    0x8
#define RAB_SLICE_ADDR_OFFSET_OFFSET_B 0x10
#define RAB_SLICE_FLAGS_OFFSET_B       0x18

#define RAB_CONFIG_N_BITS_PORT    1
#define RAB_CONFIG_N_BITS_ACP     1
#define RAB_CONFIG_N_BITS_LVL     2
#define RAB_CONFIG_N_BITS_PROT    3
#define RAB_CONFIG_N_BITS_DATE    8
#define RAB_CONFIG_N_BITS_PAGE    16

#define RAB_CONFIG_N_BITS_OFFLOAD_ID 1
#define RAB_CONFIG_N_BITS_N_ELEM     3
#define RAB_CONFIG_N_BITS_N_STRIPES  22

//#define RAB_CONFIG_CHECK_PROT     1
#define RAB_CONFIG_MAX_GAP_SIZE_B 0x1000 // one page
#define RAB_MH_ADDR_FIFO_OFFSET_B 0x0
#define RAB_MH_META_FIFO_OFFSET_B 0x8

#define PULP_SIZE_B     0x10000000
#define CLUSTER_SIZE_MB 4

#define CLUSTER_PERIPHERALS_OFFSET_B 0x200000
#define BBMUX_CLKGATE_OFFSET_B       0x800
#define EU_SW_EVENTS_OFFSET_B        0x600
#define RAB_WAKEUP_OFFSET_B          0x0

#define SOC_PERIPHERALS_SIZE_B 0x50000

#define MBOX_SIZE_B          0x1000 // Interface 0 only
#define MBOX_FIFO_DEPTH      16
#define MBOX_WRDATA_OFFSET_B 0x0
#define MBOX_RDDATA_OFFSET_B 0x8
#define MBOX_STATUS_OFFSET_B 0x10
#define MBOX_ERROR_OFFSET_B  0x14
#define MBOX_IS_OFFSET_B     0x20
#define MBOX_IE_OFFSET_B     0x24

// required for RAB miss handling
#define AXI_ID_WIDTH_CORE    4
#define AXI_ID_WIDTH_CLUSTER 3
#if PLATFORM == JUNO
  #define AXI_ID_WIDTH_SOC   3
#else // !JUNO
  #define AXI_ID_WIDTH_SOC   1
#endif
#define AXI_USER_WIDTH       6

/*
 * Platform specific settings
 */
// PLATFORM is exported in sourceme.sh and passed by the Makefile

#if PLATFORM == ZEDBOARD || PLATFORM == ZC706 || PLATFORM == MINI_ITX

  #define N_CLUSTERS       1

  // PULP address map
  #define H_GPIO_BASE_ADDR     0x51000000
  #define CLKING_BASE_ADDR     0x51010000
  #define RAB_CONFIG_BASE_ADDR 0x51030000
  //#define INTR_REG_BASE_ADDR   0x51050000 // not yet used on ZYNQ

  // IRQs
  #define END_OF_COMPUTATION_IRQ 61
  #define MBOX_IRQ               62
  #define RAB_MISS_IRQ           63
  #define RAB_MULTI_IRQ          64
  #define RAB_PROT_IRQ           65
  #define RAB_MHR_FULL_IRQ       66
  #define RAB_AR_LOG_FULL_IRQ    67
  #define RAB_AW_LOG_FULL_IRQ    68
  // #define RAB_CFG_LOG_FULL_IRQ TODO (also in HW)

  #if PLATFORM == ZEDBOARD

    #define RAB_AX_LOG_EN         0

    #define PULP_DEFAULT_FREQ_MHZ 25
    #define CLKING_INPUT_FREQ_MHZ 50

    // L3
    #define L3_MEM_SIZE_MB 16

    // Cluster + RAB config
    #define N_CORES          2
    #define L2_MEM_SIZE_KB  64
    #define L1_MEM_SIZE_KB  32
    #define RAB_L1_N_SLICES_PORT_0   4
    #define RAB_L1_N_SLICES_PORT_1   8
    // Specify for each of the RAB_N_PORTS if L2 is active on that port: {Port 0, Port 1}.
    static const unsigned RAB_L2_EN_ON_PORT[RAB_N_PORTS] = {0, 0};

    #define RAB_MH_FIFO_DEPTH 8

  #elif PLATFORM == ZC706 || PLATFORM == MINI_ITX

    #define RAB_AX_LOG_EN         1

    #define RAB_AR_LOG_BASE_ADDR  0x51100000
    #define RAB_AW_LOG_BASE_ADDR  0x51200000
    // #define RAB_CFG_LOG_BASE_ADDR TODO (also in HW)

    #define RAB_AX_LOG_SIZE_B     0x6000   // size of BRAM, 192 KiB = 2 Ki entries
    #define RAB_AX_LOG_BUF_SIZE_B 0x600000 // size of buffer in driver, 6 MiB = 512 Ki entries
    #define RAB_CFG_LOG_SIZE_B    0x30000   // size of BRAM, 196 KiB = 16 Ki entries
    #define RAB_AX_LOG_PRINT_FORMAT 0      // 0 = DEBUG, 1 = MATLAB

    #define PULP_DEFAULT_FREQ_MHZ 50
    #define CLKING_INPUT_FREQ_MHZ 100

    // L3
    #define L3_MEM_SIZE_MB 128

    // Cluster + RAB config
    #define N_CORES                  8
    #define L2_MEM_SIZE_KB         256
    #define L1_MEM_SIZE_KB         256
    #define RAB_L1_N_SLICES_PORT_0   4
    #define RAB_L1_N_SLICES_PORT_1  32
    // Specify for each of the RAB_N_PORTS if L2 is active on that port: {Port 0, Port 1}.
    static const unsigned RAB_L2_EN_ON_PORT[RAB_N_PORTS] = {0, 1};

    #define RAB_MH_FIFO_DEPTH 64

  #endif // PLATFORM

#else // JUNO

  #define N_CLUSTERS       4

  #define RAB_AX_LOG_EN    1

  // PULP address map
  #define H_GPIO_BASE_ADDR      0x6E000000
  #define CLKING_BASE_ADDR      0x6E010000
  #define RAB_CONFIG_BASE_ADDR  0x6E030000
  #define INTR_REG_BASE_ADDR    0x6E050000
  #define RAB_AR_LOG_BASE_ADDR  0x6E100000
  #define RAB_AW_LOG_BASE_ADDR  0x6E200000
  #define RAB_CFG_LOG_BASE_ADDR 0x6E300000

  #define INTR_REG_SIZE_B       0x1000
  #define RAB_AX_LOG_SIZE_B     0xC0000   // size of BRAM, 786 KiB = 64 Ki entries
  #define RAB_CFG_LOG_SIZE_B    0x30000   // size of BRAM, 196 KiB = 16 Ki entries
  #define RAB_AX_LOG_BUF_SIZE_B 0x6000000 // size of buffer in driver, 96 MiB = 8 Mi entries
  #define RAB_AX_LOG_PRINT_FORMAT 0       // 0 = DEBUG, 1 = MATLAB

  #define INTR_EOC_0              0
  #define INTR_EOC_N   N_CLUSTERS-1 // max 15

  #define INTR_MBOX              16
  #define INTR_RAB_MISS          17
  #define INTR_RAB_MULTI         18
  #define INTR_RAB_PROT          19
  #define INTR_RAB_MHR_FULL      20
  #define INTR_RAB_AR_LOG_FULL   21
  #define INTR_RAB_AW_LOG_FULL   22
  #define INTR_RAB_CFG_LOG_FULL  23

  #define PULP_DEFAULT_FREQ_MHZ 25
  #define CLKING_INPUT_FREQ_MHZ 100

  // L3
  #define L3_MEM_SIZE_MB 128

  // Cluster + RAB config
  #define N_CORES          8
  #define L2_MEM_SIZE_KB 256
  #define L1_MEM_SIZE_KB 256
  #define RAB_L1_N_SLICES_PORT_0   4
  #define RAB_L1_N_SLICES_PORT_1  32
  // Specify for each of the RAB_N_PORTS if L2 is active on that port: {Port 0, Port 1}.
  static const unsigned RAB_L2_EN_ON_PORT[RAB_N_PORTS] = {0, 1};

  #define RAB_MH_FIFO_DEPTH 64

  #define RAB_N_STATIC_2ND_LEVEL_SLICES (1 << (32 - PGDIR_SHIFT))

#endif // PLATFORM

#define CLUSTER_MASK  (unsigned)((1 << N_CLUSTERS) - 1)

/*
 * Dependent parameters
 */
#define AXI_ID_WIDTH    (AXI_ID_WIDTH_CORE + AXI_ID_WIDTH_CLUSTER + AXI_ID_WIDTH_SOC)

#define L1_MEM_SIZE_B   (L1_MEM_SIZE_KB*1024)
#define L2_MEM_SIZE_B   (L2_MEM_SIZE_KB*1024)
#define L3_MEM_SIZE_B   (L3_MEM_SIZE_MB*1024*1024)

#define CLUSTER_SIZE_B  (CLUSTER_SIZE_MB*1024*1024)
#define CLUSTERS_SIZE_B (N_CLUSTERS*CLUSTER_SIZE_B)

#define GPIO_EOC_0              0
#define GPIO_EOC_N  (N_CLUSTERS-1) // max 15

#define GPIO_RST_N          31
#define GPIO_CLK_EN         30
#define GPIO_RAB_CFG_LOG_RDY 27
#define GPIO_RAB_AR_LOG_RDY 28
#define GPIO_RAB_AW_LOG_RDY 29
#define GPIO_RAB_AR_LOG_CLR 28
#define GPIO_RAB_AW_LOG_CLR 29
#define GPIO_RAB_AX_LOG_EN  27
#define GPIO_RAB_CFG_LOG_CLR 26

// Fulmine uses Timer v.1 which has 4 core timers only
#if N_CORES > 4
  #define N_CORES_TIMER 4
#else
  #define N_CORES_TIMER N_CORES
#endif

#define MBOX_BASE_ADDR 0x1A121000  // Interface 1

/*
 * Host memory map
 */
#if PLATFORM != JUNO
  #define PULP_H_BASE_ADDR   0x40000000 // Address at which the host sees PULP
  #define L1_MEM_H_BASE_ADDR (PULP_H_BASE_ADDR)
  #define L2_MEM_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_REMOTE_ADDR + L2_MEM_BASE_ADDR)
  #define L3_MEM_H_BASE_ADDR \
    (DRAM_SIZE_MB - L3_MEM_SIZE_MB)*1024*1024 // = 0x38000000 for 1024 MB DRAM and 128 MB L3
  #define MBOX_H_BASE_ADDR \
    (PULP_H_BASE_ADDR - PULP_BASE_REMOTE_ADDR + MBOX_BASE_ADDR - MBOX_SIZE_B) // Interface 0
  #define SOC_PERIPHERALS_H_BASE_ADDR \
    (PULP_H_BASE_ADDR - PULP_BASE_REMOTE_ADDR + SOC_PERIPHERALS_BASE_ADDR)

#else // PLATFORM == JUNO
  #define PULP_H_BASE_ADDR            0x60000000 // Address at which the host sees PULP
  #define L1_MEM_H_BASE_ADDR          PULP_H_BASE_ADDR
  #define L2_MEM_H_BASE_ADDR          0x67000000
  #define L3_MEM_H_BASE_ADDR         (0xA00000000LL - L3_MEM_SIZE_B)
  #define MBOX_H_BASE_ADDR            0x65120000 // Interface 0
  #define SOC_PERIPHERALS_H_BASE_ADDR 0x65100000

#endif // PLATFORM != JUNO

#define CLUSTERS_H_BASE_ADDR (PULP_H_BASE_ADDR)
#define TIMER_H_OFFSET_B     (TIMER_BASE_ADDR - PULP_BASE_ADDR)

/*
 * Dependent parameters
 */
// RAB
#define RAB_MAX_DATE     BIT_MASK_GEN(RAB_CONFIG_N_BITS_DATE)
#define RAB_MAX_DATE_MH  (RAB_MAX_DATE-2)

// cluster peripherals, offsets compared to TCDM/clusters address
#define TIMER_START_OFFSET_B       (TIMER_H_OFFSET_B + 0x00)
#define TIMER_STOP_OFFSET_B        (TIMER_H_OFFSET_B + 0x04)
#define TIMER_RESET_OFFSET_B       (TIMER_H_OFFSET_B + 0x08)
#define TIMER_GET_TIME_LO_OFFSET_B (TIMER_H_OFFSET_B + 0x0c)
#define TIMER_GET_TIME_HI_OFFSET_B (TIMER_H_OFFSET_B + 0x10)

#define PE_TIMER_OFFSET_B         0x40

/*
 * Type Definitions
 */
// Stripe request structs - user space
typedef struct {
  unsigned short id;
  unsigned short n_elements;
  unsigned       rab_stripe_elem_user_addr; // 32b user-space addr of stripe element array
} RabStripeReqUser;

typedef enum {
  inout = 0,
  in    = 1,
  out   = 2,
} ElemType;

typedef struct {
  unsigned char id;
  ElemType type;
  unsigned char flags;
  unsigned      max_stripe_size_b;
  unsigned      n_stripes;
  unsigned      stripe_addr_start; // 32b user-space addr of addr_start array
  unsigned      stripe_addr_end;   // 32b user-space addr of addr_end array
} RabStripeElemUser;

/*
 * Program execution
 */
#define SYNC_OFFSET_B 0xB000

#define MAX_STRIPE_SIZE_B 0x2000

// needed for ROD, CT, JPEG
//#define PROFILE
//#define MEM_SHARING 2 // 1, 2 ,3
//#define ZYNQ_PMM

// needed for profile_rab_striping & profile_rab_miss_handling
#define N_CYC_TOT_RESPONSE_OFFSET_B 0x00
#define N_CYC_TOT_UPDATE_OFFSET_B   0x04
#define N_CYC_TOT_SETUP_OFFSET_B    0x08
#define N_CYC_TOT_CLEANUP_OFFSET_B  0x0c
#define N_UPDATES_OFFSET_B          0x10
#define N_SLICES_UPDATED_OFFSET_B   0x14
#define N_PAGES_SETUP_OFFSET_B      0x18
#define N_CLEANUPS_OFFSET_B         0x1c

#define N_CYC_TOT_CACHE_FLUSH_OFFSET_B    0x20
#define N_CYC_TOT_GET_USER_PAGES_OFFSET_B 0x24
#define N_CYC_TOT_MAP_SG_OFFSET_B         0x28

#define N_MISSES_OFFSET_B            0x2C
#define N_FIRST_MISSES_OFFSET_B      0x30
#define N_CYC_TOT_SCHEDULE_OFFSET_B  0x34

#define PROFILE_RAB_N_UPDATES     100000
#define PROFILE_RAB_N_REQUESTS    100
#define PROFILE_RAB_N_ELEMENTS    (PROFILE_RAB_N_REQUESTS * 10)

#endif // PULP_HOST_H___

// vim: ts=2 sw=2 sts=2 et foldmethod=marker tw=100
