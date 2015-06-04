#ifndef PULP_HOST_H___
#define PULP_HOST_H___

#include "pulp.h"
#include "pulpemu.h"

#define DEBUG_LEVEL 3

#define ZEDBOARD 1
#define ZC706    2
#define MINI_ITX 3

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

#define RAB_GET_DATE_EXP(date_exp, request) \
  ( date_exp = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, \
		      RAB_CONFIG_N_BITS_DATE) )
#define RAB_SET_DATE_EXP(request, date_exp) \
  ( BF_SET(request, date_exp, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, \
	   RAB_CONFIG_N_BITS_DATE) )

#define RAB_GET_DATE_CUR(date_cur, request) \
  ( date_cur = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		      + RAB_CONFIG_N_BITS_DATE, RAB_CONFIG_N_BITS_DATE) )
#define RAB_SET_DATE_CUR(request, date_cur) \
  ( BF_SET(request, date_cur, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	   + RAB_CONFIG_N_BITS_DATE, RAB_CONFIG_N_BITS_DATE) )

// for RAB striping - ATTENTION: not compatible with date_exp, date_cur!!!
#define RAB_GET_OFFLOAD_ID(offload_id, request) \
  ( offload_id = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, \
		    RAB_CONFIG_N_BITS_OFFLOAD_ID) )
#define RAB_SET_OFFLOAD_ID(request, offload_id) \
  ( BF_SET(request, offload_id, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, \
	   RAB_CONFIG_N_BITS_OFFLOAD_ID) )

#define RAB_GET_N_ELEM(n_elem, request) \
  ( n_elem = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		    + RAB_CONFIG_N_BITS_OFFLOAD_ID, RAB_CONFIG_N_BITS_N_ELEM) )
#define RAB_SET_N_ELEM(request, n_elem) \
  ( BF_SET(request, n_elem, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	   + RAB_CONFIG_N_BITS_OFFLOAD_ID, RAB_CONFIG_N_BITS_N_ELEM) )

#define RAB_GET_N_STRIPES(n_stripes, request) \
  ( n_stripes = BF_GET(request, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
		       + RAB_CONFIG_N_BITS_OFFLOAD_ID + RAB_CONFIG_N_BITS_N_ELEM, \
		       RAB_CONFIG_N_BITS_N_STRIPES) )
#define RAB_SET_N_STRIPES(request, n_stripes) \
  ( BF_SET(request, n_stripes, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT \
	   + RAB_CONFIG_N_BITS_OFFLOAD_ID + RAB_CONFIG_N_BITS_N_ELEM, \
	   RAB_CONFIG_N_BITS_N_STRIPES) )

  
/*
 * General settings
 */
#define PULP_N_DEV_NUMBERS 1

// ioctl setup
#define PULP_IOCTL_MAGIC 'p'
#define PULP_IOCTL_RAB_REQ   _IOR(PULP_IOCTL_MAGIC,0xB0,int)
#define PULP_IOCTL_RAB_FREE  _IOR(PULP_IOCTL_MAGIC,0xB1,int)

#define PULP_IOCTL_RAB_REQ_STRIPED  _IOR(PULP_IOCTL_MAGIC,0xB2,int)
#define PULP_IOCTL_RAB_FREE_STRIPED _IOR(PULP_IOCTL_MAGIC,0xB3,int)

#define PULP_IOCTL_RAB_MH_ENA  _IOR(PULP_IOCTL_MAGIC,0xB4,int)
#define PULP_IOCTL_RAB_MH_DIS  _IOR(PULP_IOCTL_MAGIC,0xB5,int)

#define PULP_IOCTL_DMAC_XFER _IOR(PULP_IOCTL_MAGIC,0xB6,int)

#define L3_MEM_BASE_ADDR 0x80000000

/*
 * Board selection
 */
//#define BOARD MINI_ITX/ZC706 or ZEDBOARD
#ifndef BOARD
#define BOARD MINI_ITX
#endif // BOARD

/*
 * Board specific settings
 */
#if BOARD == ZEDBOARD 

// L3
#define L3_MEM_SIZE_MB 128 
// PULP system address map
#define PULP_H_BASE_ADDR 0x40000000 // Address at which the host sees PULP
#define N_CLUSTERS 1
#define L2_MEM_SIZE_KB 64

#elif BOARD == ZC706 || BOARD == MINI_ITX

// L3
#define L3_MEM_SIZE_MB 128

// PULP system address map
#define PULP_H_BASE_ADDR 0x40000000 // Address at which the host sees PULP
#define N_CLUSTERS 1
#define N_CORES 4
#define L2_MEM_SIZE_KB 64

#endif // BOARD

/*
 * Independent parameters
 */
#define H_GPIO_BASE_ADDR 0x51000000
#define H_GPIO_SIZE_B    0x1000

#define CLKING_BASE_ADDR              0x51010000
#define CLKING_SIZE_B                 0x1000
#define CLKING_CONFIG_REG_0_OFFSET_B  0x200 
#define CLKING_CONFIG_REG_2_OFFSET_B  0x208 
#define CLKING_CONFIG_REG_23_OFFSET_B 0x25C 
#define CLKING_STATUS_REG_OFFSET_B    0x4 

#define STDOUT_H_BASE_ADDR 0x51020000
#define STDOUT_SIZE_B      0x10000
#define STDOUT_PE_SIZE_B   0x1000

#define RAB_CONFIG_BASE_ADDR      0x51030000
#define RAB_CONFIG_SIZE_B         0x1000
#define RAB_N_PORTS               2
#define RAB_N_SLICES              32
#define RAB_CONFIG_N_BITS_PORT    1
#define RAB_CONFIG_N_BITS_PROT    3
#define RAB_CONFIG_N_BITS_DATE    8
#define RAB_CONFIG_N_BITS_PAGE    16

#define RAB_CONFIG_N_BITS_OFFLOAD_ID 1
#define RAB_CONFIG_N_BITS_N_ELEM     3
#define RAB_CONFIG_N_BITS_N_STRIPES  22

//#define RAB_CONFIG_CHECK_PROT     1
#define RAB_CONFIG_MAX_GAP_SIZE_B 0x1000 // one page

#define PULP_SIZE_B     0x10000000
#define CLUSTER_SIZE_MB 4

#define SOC_PERIPHERALS_SIZE_B 0x50000

#define MAILBOX_SIZE_B          0x1000 // Interface 0 only
#define MAILBOX_FIFO_DEPTH      16
#define MAILBOX_WRDATA_OFFSET_B 0x0 
#define MAILBOX_RDDATA_OFFSET_B 0x8 
#define MAILBOX_STATUS_OFFSET_B 0x10
#define MAILBOX_ERROR_OFFSET_B  0x14
#define MAILBOX_IS_OFFSET_B     0x20
#define MAILBOX_IE_OFFSET_B     0x24

#define END_OF_COMPUTATION_IRQ 61
#define MAILBOX_IRQ            62
#define RAB_MISS_IRQ           63
#define RAB_MULTI_IRQ          64
#define RAB_PROT_IRQ           65

// old
//#define CLUSTER_CONTROLLER_OFFSET_B 0x210000
//#define MAILBOX_INTERFACE0_OFFSET_B 0x220000
//#define MAILBOX_INTERFACE1_OFFSET_B 0x221000
//#define CLUSTER_DMA_OFFSET_B        0x250000
//#define END_OF_COMPUTATION_OFFSET_B 0x260000

//#define DEMUX_CONFIG_CORE_OFFSET_B    0x1000
//#define DEMUX_CONFIG_CLUSTER_OFFSET_B 0x0

//#define CONFIG_BASE_ADDR 0x83C00000
//#define CONFIG_SIZE_B 0x401000 // something above than the highest config register
//#define DEMUX_CONFIG_BASE_ADDR 0x83C00000
//#define DEMUX_CONFIG_SIZE_B    0x1000

/*
 * Dependent parameters
 */
#define L3_MEM_SIZE_B    (L3_MEM_SIZE_MB*1024*1024)
#define L3_MEM_H_BASE_ADDR \
  (DRAM_SIZE_MB - L3_MEM_SIZE_MB)*1024*1024 // = 0x1C000000 for 512 MB DRAM and 64 MB L3

#define L2_MEM_SIZE_B      (L2_MEM_SIZE_KB*1024)
#define L2_MEM_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_ADDR + L2_MEM_BASE_ADDR)

#define SOC_PERIPHERALS_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_ADDR + SOC_PERIPHERALS_BASE_ADDR)

#define CLUSTERS_H_BASE_ADDR (PULP_H_BASE_ADDR)
#define CLUSTERS_SIZE_B      (N_CLUSTERS*CLUSTER_SIZE_MB*1024*1024)

//#define PULP_CLUSTER_OFFSET  (PULP_P_BASE_ADDR>>28)

// RAB
#define RAB_N_MHRS   (N_CLUSTERS*N_CORES)
#define RAB_MAX_DATE BIT_MASK_GEN(RAB_CONFIG_N_BITS_DATE)

// mailbox
#define MAILBOX_H_BASE_ADDR \
  (PULP_H_BASE_ADDR - PULP_BASE_ADDR + MAILBOX_BASE_ADDR - MAILBOX_SIZE_B) // Interface 0

// cluster peripherals, offsets compared to TCDM/clusters address
#define TIMER_H_OFFSET_B           (TIMER_BASE_ADDR - PULP_BASE_ADDR)
#define TIMER_START_OFFSET_B       (TIMER_H_OFFSET_B + 0x00) 
#define TIMER_STOP_OFFSET_B        (TIMER_H_OFFSET_B + 0x04) 
#define TIMER_RESET_OFFSET_B       (TIMER_H_OFFSET_B + 0x08) 
#define TIMER_GET_TIME_LO_OFFSET_B (TIMER_H_OFFSET_B + 0x0c) 
#define TIMER_GET_TIME_HI_OFFSET_B (TIMER_H_OFFSET_B + 0x10) 

/*
 * Program execution
 */
//#define BOOT_OFFSET_B 0x100
//#define BOOT_ADDR (L3_MEM_BASE_ADDR + BOOT_OFFSET_B)

#define SYNC_OFFSET_B 0xB000

// needed for ROD, CT, JPEG
//#define ROD
//#define CT
//#define JPEG
//#define MEM_SHARING 2
//#define ZYNQ_PMM
//#define PROFILE

// needed for profile_rab, dma_test
//#define MEM_SHARING 2
//#define PROFILE_RAB
//#define MAX_STRIPE_SIZE 0x1800

// needed for profile_rab
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

#endif // PULP_HOST_H___
