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
#ifndef ZYNQMP_H___
#define ZYNQMP_H___

#include "arm64.h"

/*
 * Independent parameters
 */
#define ARM_CLK_FREQ_MHZ 1200
#define DRAM_SIZE_MB 2048

// System Memory Management Unit
#define SMMU_BASE_ADDR      0xFD800000
#define SMMU_SIZE_B         0x20000
#define SMMU_CB_OFFSET_B    0x10000
#define SMMU_CB_SIZE_B      0x1000

#define SMMU_N_SMRS         48

#define SMMU_STREAM_ID_HPC0 0x200
#define SMMU_STREAM_ID_HP0  0xE80

// UART1 Controller - PULP -> Host UART
#define HOST_UART_BASE_ADDR      0xFF010000
#define HOST_UART_SIZE_B         0x1000
#define MODEM_CTRL_REG0_OFFSET_B 0x24

#endif // ZYNQMP_H___
