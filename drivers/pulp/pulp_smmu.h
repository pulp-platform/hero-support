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
#ifndef _PULP_SMMU_H_
#define _PULP_SMMU_H_

#include <linux/iommu.h>    /* iommu stuff */

#include "pulp_module.h"

// constants
#define SMMU_SMR_OFFSET_B       0x800
#define SMMU_S2CR_OFFSET_B      0xC00
#define SMMU_CBAR_OFFSET_B      0x1000

#define SMMU_CB_SCTLR_OFFSET_B  0x0
#define SMMU_CB_RESUME_OFFSET_B 0x8
#define SMMU_CB_MAIR0_OFFSET_B  0x38
#define SMMU_CB_MAIR1_OFFSET_B  0x3C
#define SMMU_CB_FSR_OFFSET_B    0x58

#define SMMU_S2CR_SHCFG         8
#define SMMU_S2CR_MTCFG         11
#define SMMU_S2CR_MEMATTR       12
#define SMMU_S2CR_TYPE          16
#define SMMU_S2CR_NSCFG         18
#define SMMU_S2CR_RACFG         20
#define SMMU_S2CR_WACFG         22
#define SMMU_S2CR_TRANSIENTCFG  28

#define SMMU_CBAR_BPSHCFG       8

#define SMMU_CB_FSR_TF          1

#define SMMU_SCTLR_CFIE        (1 << 6)

#define SMMU_N_BITS_STREAM_ID   14

// type definitions
typedef enum {
  READY = 0,
  WAIT  = 1,
} WorkerStatus;

// methods declarations
int pulp_smmu_init(PulpDev * pulp_ptr);

int pulp_smmu_ena(PulpDev * pulp_ptr, unsigned flags);
int pulp_smmu_dis(PulpDev * pulp_ptr);

int pulp_smmu_fh_sched(struct iommu_domain *smmu_domain_ptr, struct device *dev_ptr,
                       unsigned long iova, int flags, void * smmu_token_ptr);
void pulp_smmu_handle_fault(void);

#endif/*_PULP_SMMU_H_*/
