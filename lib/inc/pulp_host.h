#ifndef PULP_HOST_H___
#define PULP_HOST_H___

#include "../../../../hardware/pulp2/sw/libs/sys_lib/inc/pulp.h"

#define ZEDBOARD 1
#define ZC706 2

#define DEBUG_LEVEL 0

/*
 * General settings
 */
#define PULP_N_DEV_NUMBERS 1

/*
 * Board selection
 */
//#define BOARD ZC706 or ZEDBOARD
#ifndef BOARD
#define BOARD ZC706
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

#elif BOARD == ZC706

// L3
#define L3_MEM_SIZE_MB 128

// PULP system address map
//#define PULP_H_BASE_ADDR 0x40000000 // Address at which the host sees PULP
#define PULP_H_BASE_ADDR 0x43C00000 // Address at which the host sees PULP
#define N_CLUSTERS 1
#define L2_MEM_SIZE_KB 64

#endif // BOARD

/*
 * Independent parameters
 */
#define RAB_CONFIG_BASE_ADDR 0x43C10000
#define RAB_CONFIG_SIZE_B    0x1000
#define RAB_N_PORTS          3
#define RAB_N_SLICES         16

#define H_GPIO_BASE_ADDR 0x41200000
#define H_GPIO_SIZE_B    0x1000

#define PULP_SIZE_B     0x10000000
#define CLUSTER_SIZE_MB 4

#define SOC_PERIPHERALS_SIZE_B 0x50000

//#define CLUSTER_CONTROLLER_OFFSET_B 0x210000
//#define MAILBOX_INTERFACE0_OFFSET_B 0x220000
//#define MAILBOX_INTERFACE1_OFFSET_B 0x221000
//#define CLUSTER_DMA_OFFSET_B        0x250000
//#define END_OF_COMPUTATION_OFFSET_B 0x260000

//#define DEMUX_CONFIG_CORE_OFFSET_B    0x1000
//#define DEMUX_CONFIG_CLUSTER_OFFSET_B 0x0

//#define MAILBOX_WRDATA_OFFSET_B 0x0
//#define MAILBOX_RDDATA_OFFSET_B 0x8
//#define MAILBOX_STATUS_OFFSET_B 0x10
//#define MAILBOX_ERROR_OFFSET_B  0x14
//#define MAILBOX_IS_OFFSET_B     0x20
//#define MAILBOX_IE_OFFSET_B     0x24

//#define END_OF_COMPUTATION_IRQ 61
//#define RAB_MISS_IRQ           62
//#define MAILBOX_INTERFACE0_IRQ 63
//#define MAILBOX_INTERFACE1_IRQ 64

//#define CONFIG_BASE_ADDR 0x83C00000
//#define CONFIG_SIZE_B 0x401000 // something above than the highest config register
//#define DEMUX_CONFIG_BASE_ADDR 0x83C00000
//#define DEMUX_CONFIG_SIZE_B    0x1000

/*
 * Dependent parameters
 */
#define L3_MEM_SIZE_B (L3_MEM_SIZE_MB*1024*1024)
#define L3_MEM_BASE_ADDR (DRAM_SIZE_MB - L3_MEM_SIZE_MB)*1024*1024 // = 0x1C000000 for 512 MB DRAM and 64 MB L3

#define L2_MEM_SIZE_B (L2_MEM_SIZE_KB*1024)
#define L2_MEM_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_ADDR + L2_MEM_BASE_ADDR)

#define SOC_PERIPHERALS_H_BASE_ADDR (PULP_H_BASE_ADDR - PULP_BASE_ADDR + SOC_PERIPHERALS_BASE_ADDR)

#define CLUSTERS_H_BASE_ADDR (PULP_H_BASE_ADDR)
#define CLUSTERS_SIZE_B (N_CLUSTERS*CLUSTER_SIZE_MB*1024*1024)

#define PULP_CLUSTER_OFFSET (PULP_P_BASE_ADDR>>28)

/*
 * Program execution
 */
#define BOOT_OFFSET_B 0x100
#define BOOT_ADDR (L3_MEM_BASE_ADDR + BOOT_OFFSET_B)

#endif // PULP_HOST_H___
