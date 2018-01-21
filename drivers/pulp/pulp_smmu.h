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

// type definitions
typedef enum {
  READY = 0,
  START = 1,
  BUSY  = 2,
  DONE  = 3,
  ERROR = 4,
} WorkerStatus;

// methods declarations
int pulp_smmu_init(PulpDev * pulp_ptr);

int pulp_smmu_ena(PulpDev * pulp_ptr, unsigned flags);
int pulp_smmu_dis(PulpDev * pulp_ptr);

int pulp_smmu_fh_sched(struct iommu_domain *smmu_domain_ptr, struct device *dev_ptr,
                       unsigned long iova, int flags, void * smmu_token_ptr);
void pulp_smmu_handle_fault(void);

#endif/*_PULP_SMMU_H_*/
