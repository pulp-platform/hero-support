#ifndef PULP_HOST_H___
#define PULP_HOST_H___

#include "pulp.h"
#include "pulpemu.h"

#define DEBUG_LEVEL 0

// PLATFORM is exported in sourceme.sh and passed by the Makefile
#define ZEDBOARD 1
#define ZC706    2
#define MINI_ITX 3
#define JUNO     4

// mailbox communication
#define PULP_READY 0x0
#define PULP_START 0x1
#define PULP_BUSY  0x2
#define PULP_DONE  0x3 
#define PULP_STOP  0xF 

#define HOST_READY 0x1000
//#define HOST_START 0x1001
//#define HOST_BUSY  0x1002
#define HOST_DONE  0x1003

#define TO_RUNTIME 0x10000000 // pass PULP driver
#define RAB_UPDATE 0x10000001 // handled by PULP driver
#define RAB_SWITCH 0x10000002 // handled by PULP driver

/*
 * Macros
 */
#define BIT_N_SET(n)        ( 1 << (n) )
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
#define RAB_GET_PROT(prot, request) ( prot = request & 0x7 )
#define RAB_SET_PROT(request, prot) \
  ( BF_SET(request, prot, 0, RAB_CONFIG_N_BITS_PROT) )

#define RAB_GET_PORT(port, request) \
  ( port = BF_GET(request, RAB_CONFIG_N_BITS_PROT, RAB_CONFIG_N_BITS_PORT) )
#define RAB_SET_PORT(request, port) \
  ( BF_SET(request, port, RAB_CONFIG_N_BITS_PROT, RAB_CONFIG_N_BITS_PORT) )

#define RAB_GET_USE_ACP(use_acp, request) \
  ( use_acp = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, \
		      RAB_CONFIG_N_BITS_USE_ACP) )
#define RAB_SET_USE_ACP(request, use_acp) \
  ( BF_SET(request, use_acp, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, \
	   RAB_CONFIG_N_BITS_USE_ACP) )

#define RAB_GET_DATE_EXP(date_exp, request) \
  ( date_exp = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
           + RAB_CONFIG_N_BITS_USE_ACP, RAB_CONFIG_N_BITS_DATE) )
#define RAB_SET_DATE_EXP(request, date_exp) \
  ( BF_SET(request, date_exp, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	         + RAB_CONFIG_N_BITS_USE_ACP, RAB_CONFIG_N_BITS_DATE) )

#define RAB_GET_DATE_CUR(date_cur, request) \
  ( date_cur = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		       + RAB_CONFIG_N_BITS_DATE + RAB_CONFIG_N_BITS_USE_ACP, RAB_CONFIG_N_BITS_DATE) )
#define RAB_SET_DATE_CUR(request, date_cur) \
  ( BF_SET(request, date_cur, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	         + RAB_CONFIG_N_BITS_DATE + RAB_CONFIG_N_BITS_USE_ACP, RAB_CONFIG_N_BITS_DATE) )

// for RAB striping - ATTENTION: not compatible with date_exp, date_cur!!!
#define RAB_GET_OFFLOAD_ID(offload_id, request) \
  ( offload_id = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
          + RAB_CONFIG_N_BITS_USE_ACP, RAB_CONFIG_N_BITS_OFFLOAD_ID) )
#define RAB_SET_OFFLOAD_ID(request, offload_id) \
  ( BF_SET(request, offload_id, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
          + RAB_CONFIG_N_BITS_USE_ACP, RAB_CONFIG_N_BITS_OFFLOAD_ID) )

#define RAB_GET_N_ELEM(n_elem, request) \
  ( n_elem = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		     + RAB_CONFIG_N_BITS_USE_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID, RAB_CONFIG_N_BITS_N_ELEM) )
#define RAB_SET_N_ELEM(request, n_elem) \
  ( BF_SET(request, n_elem, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	       + RAB_CONFIG_N_BITS_USE_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID, RAB_CONFIG_N_BITS_N_ELEM) )

#define RAB_GET_N_STRIPES(n_stripes, request) \
  ( n_stripes = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		     + RAB_CONFIG_N_BITS_USE_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID + RAB_CONFIG_N_BITS_N_ELEM, \
		     RAB_CONFIG_N_BITS_N_STRIPES) )
#define RAB_SET_N_STRIPES(request, n_stripes) \
  ( BF_SET(request, n_stripes, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	       + RAB_CONFIG_N_BITS_USE_ACP + RAB_CONFIG_N_BITS_OFFLOAD_ID + RAB_CONFIG_N_BITS_N_ELEM, \
	       RAB_CONFIG_N_BITS_N_STRIPES) )

/*
 * General settings
 */
#define PULP_N_DEV_NUMBERS 1

// ioctl setup
#define PULP_IOCTL_MAGIC 'p'
#define PULP_IOCTL_RAB_REQ   _IOW(PULP_IOCTL_MAGIC,0xB0,unsigned) // ptr
#define PULP_IOCTL_RAB_FREE  _IOW(PULP_IOCTL_MAGIC,0xB1,unsigned) // value

#define PULP_IOCTL_RAB_REQ_STRIPED  _IOW(PULP_IOCTL_MAGIC,0xB2,unsigned) // ptr
#define PULP_IOCTL_RAB_FREE_STRIPED _IOW(PULP_IOCTL_MAGIC,0xB3,unsigned) // value

#define PULP_IOCTL_RAB_MH_ENA  _IOW(PULP_IOCTL_MAGIC,0xB4,unsigned) // value
#define PULP_IOCTL_RAB_MH_DIS  _IO(PULP_IOCTL_MAGIC,0xB5)

#define PULP_IOCTL_DMAC_XFER _IOW(PULP_IOCTL_MAGIC,0xB6,unsigned) // ptr

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
  //#define TRACE_CTRL_BASE_ADDR 0x51040000 // not yet used
  //#define INTR_REG_BASE_ADDR   0x51050000 // not yet used on ZYNQ

  // IRQs
  #define END_OF_COMPUTATION_IRQ 61
  #define MBOX_IRQ               62
  #define RAB_MISS_IRQ           63
  #define RAB_MULTI_IRQ          64
  #define RAB_PROT_IRQ           65
  //#define RAB_MHR_FULL_IRQ       66 // not yet used

  #define PULP_DEFAULT_FREQ_MHZ 50

  #if PLATFORM == ZEDBOARD

    #define CLKING_INPUT_FREQ_MHZ 50

    // L3
    #define L3_MEM_SIZE_MB 8
    
    // Cluster + RAB config
    #define N_CORES          2
    #define L2_MEM_SIZE_KB  64
    #define L1_MEM_SIZE_KB  36
    #define RAB_N_SLICES    12
  
  #elif PLATFORM == ZC706 || PLATFORM == MINI_ITX
  
    #define CLKING_INPUT_FREQ_MHZ 100

    // L3
    #define L3_MEM_SIZE_MB 128
    
    // Cluster + RAB config
    #define N_CORES 4
    #define L2_MEM_SIZE_KB 256
    #define L1_MEM_SIZE_KB 72
    #define RAB_N_SLICES   32
  
  #endif // PLATFORM

#else // JUNO

  #define N_CLUSTERS       4

  // PULP address map
  #define H_GPIO_BASE_ADDR     0x6E000000
  #define CLKING_BASE_ADDR     0x6E010000
  #define RAB_CONFIG_BASE_ADDR 0x6E030000
  //#define TRACE_CTRL_BASE_ADDR 0x6E040000 // not yet used
  #define INTR_REG_BASE_ADDR   0x6E050000

  #define INTR_REG_SIZE_B 0x1000

  #define INTR_EOC_0              0
  #define INTR_EOC_N   N_CLUSTERS-1 // max 15

  #define INTR_MBOX              16
  #define INTR_RAB_MISS          17
  #define INTR_RAB_MULTI         18
  #define INTR_RAB_PROT          19
  #define INTR_RAB_MHR_FULL      20

  #define PULP_DEFAULT_FREQ_MHZ 25
  #define CLKING_INPUT_FREQ_MHZ 100

  // L3
  #define L3_MEM_SIZE_MB 128

  // Cluster + RAB config
  #define N_CORES 8
  #define L2_MEM_SIZE_KB 256
  #define L1_MEM_SIZE_KB 72
  #define RAB_N_SLICES   32

#endif // PLATFORM

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

#define RAB_CONFIG_SIZE_B         0x1000
#define RAB_N_MAPPINGS            2
#define RAB_N_PORTS               2
#define RAB_CONFIG_N_BITS_PORT    1
#define RAB_CONFIG_N_BITS_USE_ACP 1
#define RAB_CONFIG_N_BITS_PROT    3
#define RAB_CONFIG_N_BITS_DATE    8
#define RAB_CONFIG_N_BITS_PAGE    16

#define RAB_CONFIG_N_BITS_OFFLOAD_ID 1
#define RAB_CONFIG_N_BITS_N_ELEM     3
#define RAB_CONFIG_N_BITS_N_STRIPES  22

//#define RAB_CONFIG_CHECK_PROT     1
#define RAB_CONFIG_MAX_GAP_SIZE_B 0x1000 // one page
#define RAB_MH_ADDR_FIFO_OFFSET_B 0x0
#define RAB_MH_ID_FIFO_OFFSET_B   0x4
#define RAB_MH_FIFO_DEPTH         16

#define PULP_SIZE_B     0x10000000
#define CLUSTER_SIZE_MB 4

#define CLUSTER_PERIPHERALS_OFFSET_B 0x200000
#define BBMUX_CLKGATE_OFFSET_B       0x800
#define GP_1_OFFSET_B                0x364
#define GP_2_OFFSET_B                0x368

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
#define AXI_ID_WIDTH         10
#define AXI_ID_WIDTH_CORE    4
#define AXI_ID_WIDTH_CLUSTER 2
#define AXI_ID_WIDTH_SOC     3

/*
 * Dependent parameters
 */
#define L1_MEM_SIZE_B   (L1_MEM_SIZE_KB*1024)
#define L2_MEM_SIZE_B   (L2_MEM_SIZE_KB*1024)
#define L3_MEM_SIZE_B   (L3_MEM_SIZE_MB*1024*1024)

#define CLUSTER_SIZE_B  (CLUSTER_SIZE_MB*1024*1024)
#define CLUSTERS_SIZE_B (N_CLUSTERS*CLUSTER_SIZE_B)

/*
 * Host memory map 
 */
#if PLATFORM != JUNO
  #define PULP_H_BASE_ADDR   0x40000000 // Address at which the host sees PULP
  #define L1_MEM_H_BASE_ADDR (PULP_H_BASE_ADDR)
  #define L2_MEM_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_ADDR + L2_MEM_BASE_ADDR)  
  #define L3_MEM_H_BASE_ADDR \
    (DRAM_SIZE_MB - L3_MEM_SIZE_MB)*1024*1024 // = 0x38000000 for 1024 MB DRAM and 128 MB L3
  #define MBOX_H_BASE_ADDR \
    (PULP_H_BASE_ADDR - PULP_BASE_ADDR + MBOX_BASE_ADDR - MBOX_SIZE_B) // Interface 0 
  #define SOC_PERIPHERALS_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_ADDR + SOC_PERIPHERALS_BASE_ADDR)

#else // PLATFORM == JUNO
  #define PULP_H_BASE_ADDR            0x60000000 // Address at which the host sees PULP
  #define L1_MEM_H_BASE_ADDR          PULP_H_BASE_ADDR
  #define L2_MEM_H_BASE_ADDR          0x67000000
  #define L3_MEM_H_BASE_ADDR         (0x1000000000LL - L3_MEM_SIZE_B)
  #define MBOX_H_BASE_ADDR            0x65120000 // Interface 0
  #define SOC_PERIPHERALS_H_BASE_ADDR 0x65100000
  
#endif // PLATFORM != JUNO

// Redefine MBOX_BASE_ADDR -> edit pulpemu.h!!!!
#ifdef MBOX_BASE_ADDR
  #undef MBOX_BASE_ADDR
#endif
#define MBOX_BASE_ADDR 0x1A121000 // Interface 1

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
 * Program execution
 */
#define SYNC_OFFSET_B 0xB000

#define MAX_STRIPE_SIZE 0x4000

//#define MEM_SHARING 3

// needed for ROD, CT, JPEG
//#define PROFILE
//#define MEM_SHARING 2 // 1, 2 ,3
//#define ZYNQ_PMM

// needed for profile_rab, dma_test
//#define MEM_SHARING 2
//#define PROFILE_RAB

// needed for profile_rab & profile_rab_mh
#define CLK_CNTR_RESPONSE_OFFSET_B 0x00
#define CLK_CNTR_UPDATE_OFFSET_B   0x04
#define CLK_CNTR_SETUP_OFFSET_B    0x08
#define CLK_CNTR_CLEANUP_OFFSET_B  0x0c
#define N_UPDATES_OFFSET_B         0x10
#define N_SLICES_UPDATED_OFFSET_B  0x14
#define N_PAGES_SETUP_OFFSET_B     0x18
#define N_CLEANUPS_OFFSET_B        0x1c

#define CLK_CNTR_CACHE_FLUSH_OFFSET_B    0x20
#define CLK_CNTR_GET_USER_PAGES_OFFSET_B 0x24
#define CLK_CNTR_MAP_SG_OFFSET_B         0x28

#define N_MISSES_OFFSET_B        0x2C
#define N_FIRST_MISSES_OFFSET_B  0x30
#define CLK_CNTR_REFILL_OFFSET_B 0x40
#define CLK_CNTR_SCHED_OFFSET_B  0x50
#define CLK_CNTR_RESP_OFFSET_B   0x60

#endif // PULP_HOST_H___
