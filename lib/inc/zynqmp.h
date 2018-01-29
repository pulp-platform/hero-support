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
#define SMMU_BASE_ADDR     0xFD800000
#define SMMU_SIZE_B        0x20000
#define SMMU_SMR_OFFSET_B  0x800
#define SMMU_S2CR_OFFSET_B 0xC00

#define SMMU_S2CR_SHCFG         8
#define SMMU_S2CR_MTCFG        11
#define SMMU_S2CR_MEMATTR      12
#define SMMU_S2CR_TYPE         16
#define SMMU_S2CR_NSCFG        18
#define SMMU_S2CR_RACFG        20
#define SMMU_S2CR_WACFG        22
#define SMMU_S2CR_TRANSIENTCFG 28

#define SMMU_CBAR_OFFSET_B 0x1000
#define SMMU_CBAR_SIZE_B   0x4

#define SMMU_CBAR_BPSHCFG 8

#define SMMU_CB_OFFSET_B        0x10000
#define SMMU_CB_SIZE_B          0x1000
#define SMMU_CB_SCTLR_OFFSET_B  0x0
#define SMMU_CB_RESUME_OFFSET_B 0x8
#define SMMU_CB_MAIR0_OFFSET_B  0x38
#define SMMU_CB_MAIR1_OFFSET_B  0x3C
#define SMMU_CB_FSR_OFFSET_B    0x58

#define SMMU_CB_FSR_TF 1

#define SMMU_N_SMRS           48
#define SMMU_N_BITS_STREAM_ID 14
#define SMMU_STREAM_ID_HPC0   0x200
#define SMMU_STREAM_ID_HP0    0xE80

#define SMMU_SCTLR_CFIE       (1 << 6)

#endif // ZYNQMP_H___
