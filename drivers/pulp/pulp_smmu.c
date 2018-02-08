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
#include <linux/spinlock.h>
#include <linux/delay.h>           /* msleep */
#include <asm/io.h>                /* ioremap, iounmap, iowrite32 */
#include <linux/platform_device.h> /* for device tree stuff*/


#include "pulp_smmu.h"
#include "pulp_rab.h"
#include "pulp_mem.h"       /* for cache invalidation */

#include "pulp_module.h"

// global variables {{{
static struct iommu_domain * smmu_domain_ptr;
static PulpDev * pulp;

static int smr_ids[2];
static unsigned int cbndx;
static unsigned int sctlr;

// for fault handler worker
static char   smmu_fh_wq_name[11] = "SMMU_FH_WQ";
static struct workqueue_struct *smmu_fh_wq = 0;
static struct work_struct       smmu_fh_w;

static int   smmu_token[16];
static void *smmu_token_ptr = (void *)&smmu_token;

static struct task_struct *user_task;
static struct mm_struct   *user_mm;

// lock for synchronization of top and bottom half of fault handler
DEFINE_SPINLOCK(smmu_fault_lock);
static WorkerStatus  smmu_fault_status;
static unsigned long iova_faulty;

// fault handler config
static unsigned coherent;

// very simple managment structures
#define N_PAGES 256
static unsigned long iova_array[N_PAGES];
static struct page  *pages_ptrs[N_PAGES];
static unsigned int  page_idx = 0;

static size_t size = PAGE_SIZE;
//}}}

// init {{{
/**
 * Configure S2CR attributes.
 *
 * This function sets various attributes in the S2CR for the HPC0 and HP0 streams. For details,
 * check ARM SMMU Architecture Specification 2.0 Sections 2.4, 9.4. and 9.6.23.
 *
 * Note: The CCI does not accept inner- or outer-shareable transactions on the Xilinx ZynqMPSoC.
 * Thus, the shareability is set to 0b00 (default) in bypass mode. If the SMMU is not in bypass
 * mode, the effective shareability is obtained from the page table and the CBAR.
 *
 * @param   pulp_ptr Pointer to PulpDev structure.
 *
 * @return  0 on success, a nonzero errno on errros.
 */
static int pulp_smmu_set_attr(PulpDev * pulp_ptr)
{
  unsigned int offset, value;

  if ( (smr_ids[0] == SMMU_N_SMRS) || (smr_ids[1] == SMMU_N_SMRS) ) {
    printk(KERN_WARNING "PULP - SMMU: SMR IDs invalid. Cannot set attributes.\n");
    return -EINVAL;
  }

  // modify S2CR register for HPC0 stream
  offset = SMMU_S2CR_OFFSET_B + smr_ids[0]*4;
  value  = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));

  // set shareability - default: 0b00, outer shareable: 0b01, inner shareable: 0b10
  BF_SET(value, 0b00, SMMU_S2CR_SHCFG, 2);

  // set MemAttr
  BF_SET(value, 0b1, SMMU_S2CR_MTCFG, 1); // use MemAttr
  BF_SET(value, 0b1111, SMMU_S2CR_MEMATTR, 4); // outer + inner write-back cacheable

  // set NSCFG
  BF_SET(value, 0b11, SMMU_S2CR_NSCFG, 2); // non-secure

  // set RA/WA
  BF_SET(value, 0b00, SMMU_S2CR_RACFG, 2); // default - take over from AxCACHE
  BF_SET(value, 0b00, SMMU_S2CR_WACFG, 2); // default - take over from AxCACHE

  // set transient hint
  BF_SET(value, 0b10, SMMU_S2CR_TRANSIENTCFG, 2); // non-transient

  iowrite32(value, (void *)((unsigned long)pulp_ptr->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to S2CR%i\n", value, smr_ids[0]);

  // modify S2CR register for HP0 stream
  offset = SMMU_S2CR_OFFSET_B + smr_ids[1]*4;
  value  = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));

  // set shareability - default: 0b00, outer shareable: 0b01, inner shareable: 0b10
  BF_SET(value, 0b00, SMMU_S2CR_SHCFG, 2);

  // set MemAttr
  BF_SET(value, 0b1, SMMU_S2CR_MTCFG, 1); // use MemAttr
  BF_SET(value, 0b0101, SMMU_S2CR_MEMATTR, 4); // outer + inner non-cacheable

  // set NSCFG
  BF_SET(value, 0b11, SMMU_S2CR_NSCFG, 2); // non-secure

  // set RA/WA
  BF_SET(value, 0b00, SMMU_S2CR_RACFG, 2); // default - take over from AxCACHE?
  BF_SET(value, 0b00, SMMU_S2CR_WACFG, 2); // default - take over from AxCACHE?

  // set transient hint
  BF_SET(value, 0b10, SMMU_S2CR_TRANSIENTCFG, 2); // non-transient

  iowrite32(value, (void *)((unsigned long)pulp_ptr->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to S2CR%i\n", value, smr_ids[1]);

  return 0;
}

/**
 * Enable bypassing of SMMU.
 *
 * This function enables the bypassing for the HPC0 and HP0 streams.
 *
 * @param   pulp_ptr Pointer to PulpDev structure.
 *
 * @return  0 on success, a nonzero errno on errros.
 */
static int pulp_smmu_bypass(PulpDev * pulp_ptr)
{
  unsigned int offset, value;

  if ( (smr_ids[0] == SMMU_N_SMRS) || (smr_ids[1] == SMMU_N_SMRS) ) {
    printk(KERN_WARNING "PULP - SMMU: SMR IDs invalid. Cannot enable bypassing.\n");
    return -EINVAL;
  }

  // modify S2CR register for HPC0 stream
  offset = SMMU_S2CR_OFFSET_B + smr_ids[0]*4;
  value  = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));

  // set type
  BF_SET(value, 0b01, SMMU_S2CR_TYPE, 2); // bypass

  iowrite32(value, (void *)((unsigned long)pulp_ptr->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to S2CR%i\n", value, smr_ids[0]);

  // modify S2CR register for HP0 stream
  offset = SMMU_S2CR_OFFSET_B + smr_ids[1]*4;
  value  = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));

  // set type
  BF_SET(value, 0b01, SMMU_S2CR_TYPE, 2); // bypass

  iowrite32(value, (void *)((unsigned long)pulp_ptr->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to S2CR%i\n", value, smr_ids[1]);

  return 0;
}

/**
 * Initialize SMMU.
 *
 * This function initializes the SMMU. In particular, it extracts the stream matching registers
 * and entries assigned at startup based on the known stream IDs, and then enables bypassing for
 * these stream matching entries.
 *
 * @param   pulp_ptr Pointer to PulpDev structure.
 *
 * @return  0 on success, a nonzero errno on errros.
 */
int pulp_smmu_init(PulpDev * pulp_ptr)
{
  unsigned int offset, value, i, stream_id, valid, ret;
  unsigned int smr_ids_found[2];

  smr_ids_found[0] = 0;
  smr_ids_found[1] = 0;

  smr_ids[0] = SMMU_N_SMRS;
  smr_ids[1] = SMMU_N_SMRS;

  // find the stream matching registers and entries assigned at startup
  offset = SMMU_SMR_OFFSET_B;
  for (i = 0; i<SMMU_N_SMRS; i++) {
    offset += i*4;
    value = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));
    valid = BF_GET(value, 31, 1);
    stream_id = BF_GET(value, 0, SMMU_N_BITS_STREAM_ID);

    if (DEBUG_LEVEL_SMMU > 2)
      printk(KERN_INFO "PULP - SMMU: SMR %i: valid = %i, stream_id = %#x\n", i, valid, stream_id);

    // match stream IDs
    if (valid) {
      if      (stream_id == SMMU_STREAM_ID_HPC0) {
        smr_ids[0] = i;
        if (smr_ids_found[0]) {
          printk(KERN_WARNING "PULP - SMMU: Found second SMR with Stream ID %#x.\n", stream_id);
          return -ENOENT;
        }
        else
          smr_ids_found[0] = 1;
      }
      else if (stream_id == SMMU_STREAM_ID_HP0 ) {
        smr_ids[1] = i;
        if (smr_ids_found[1]) {
          printk(KERN_WARNING "PULP - SMMU: Found second SMR with Stream ID %#x.\n", stream_id);
          return -ENOENT;
        }
        else
          smr_ids_found[1] = 1;
      }
    }
  }

  // check success
  if ( !smr_ids_found[0] || !smr_ids_found[1] ) {
    printk(KERN_WARNING "PULP - SMMU: Could not identify SMRs. Check device tree parsing.\n");
    return -ENOENT;
  }

  // set attributes
  ret = pulp_smmu_set_attr(pulp_ptr);
  if (ret) {
    return ret;
  }

  // enable bypassing
  ret = pulp_smmu_bypass(pulp_ptr);

  return ret;
}
// }}}

// enable_disable {{{
/***********************************************************************************
 *
 * ███████╗███╗   ██╗ █████╗     ██╗██████╗ ██╗███████╗
 * ██╔════╝████╗  ██║██╔══██╗   ██╔╝██╔══██╗██║██╔════╝
 * █████╗  ██╔██╗ ██║███████║  ██╔╝ ██║  ██║██║███████╗
 * ██╔══╝  ██║╚██╗██║██╔══██║ ██╔╝  ██║  ██║██║╚════██║
 * ███████╗██║ ╚████║██║  ██║██╔╝   ██████╔╝██║███████║
 * ╚══════╝╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝    ╚═════╝ ╚═╝╚══════╝
 *
 ***********************************************************************************/
/**
 * Enable SMMU.
 *
 * This function enables and sets up the SMMU for virtual-to-physical address translation for PULP.
 * To this end, it disables all user-space mappings in the RAB, sets up the RAB for bypassing and
 * sets up the SMMU through the Linux IOMMU API.
 *
 * @param   pulp_ptr Pointer to PulpDev structure.
 * @param   flags    Control flags for the RAB and SMMU.
 *
 * @return  0 on success, a nonzero errno on errros.
 */
int pulp_smmu_ena(PulpDev *pulp_ptr, unsigned flags)
{
  int ret, i ,j;
  int data = 0; // arm_smmu_domain_set_attr requires 0 to set ARM_SMMU_DOMAIN_S1
  unsigned offset, value;
  RabSliceReq rab_slice_req;

  /*
   * init iova array
   */
  for (i=N_PAGES-1; i>=0; i--) {
    iova_array[i] = 0;
  }

  /*
   * prepare RAB for bypassing
   */
  // disable all existing mappings to user-space virtual memory
  ret = pulp_rab_release();
  if (ret) {
    printk(KERN_WARNING "PULP - SMMU: Failed to prepare RAB.\n");
    return ret;
  }

  // store flags
  coherent = flags;

  /*
   * set up smmu_domain
   */
  // allocate new domain
  smmu_domain_ptr = iommu_domain_alloc(&platform_bus_type);

  // set attributes
  ret = iommu_domain_set_attr(smmu_domain_ptr, DOMAIN_ATTR_NESTING, (void *)&data);
  if (ret) {
    printk(KERN_INFO "PULP - SMMU: iommu_domain_set_attr() failed: %d\n", ret);
    return ret;
  }

  // register fault worker thread with CWM API - single-threaded workqueue (bottom half)
  smmu_fh_wq = alloc_workqueue("%s", WQ_UNBOUND | WQ_HIGHPRI, 1, smmu_fh_wq_name);
  if (smmu_fh_wq == NULL) {
    printk(KERN_WARNING "PULP - SMMU: Allocation of workqueue for SMMU fault handling failed.\n");
    return -ENOMEM;
  }

  // initialize the workqueue
  pulp = pulp_ptr;
  smmu_fault_status = READY;
  INIT_WORK(&smmu_fh_w, (void *)pulp_smmu_handle_fault);

  // prepare fault handler
  user_task = current;
  user_mm = current->mm;

  // set up fault handler (top half)
  iommu_set_fault_handler(smmu_domain_ptr, (iommu_fault_handler_t)&pulp_smmu_fh_sched, smmu_token_ptr);

  // finally attach the domain to the device
  ret = iommu_attach_device(smmu_domain_ptr, pulp_ptr->dt_dev_ptr);
  if (ret) {
    printk(KERN_WARNING "PULP - SMMU: Failed to attach IOMMU domain to device: %d.\n", ret);
    return ret;
  }

  // get context bank ID for top/bottom half
  offset = SMMU_S2CR_OFFSET_B + smr_ids[0]*4;
  value  = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));
  cbndx  = BF_GET(value, 0, 8);
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: cbndx = %i\n", cbndx);

  offset = SMMU_S2CR_OFFSET_B + smr_ids[1]*4;
  value  = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));
  cbndx  = BF_GET(value, 0, 8);
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: cbndx = %i\n", cbndx);

  // get the value of the SCTLR to restore in bottom half
  offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_SCTLR_OFFSET_B;
  sctlr = ioread32((void *)((unsigned long)pulp_ptr->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: sctlr = %#x\n", sctlr);

  /*
   * configure SMMU attributes
   *
   * For details, check ARM SMMU Architecture Specification 2.0 Sections 2.4, 9.4. and 9.6.23.
   *
   * Note: The CCI does not accept inner- or outer-shareable transactions on the Xilinx ZynqMPSoC.
   * The effective shareability is obtained from the page table + (MAIR0, MAIR1) and the CBAR if
   * the SMMU is not in bypass mode.
   *
   * Besides setting these registers appropriately, also the kernel (io-pgtable-arm.c) needs to
   * be patched: the PTEs in the I/O page table need to be allocated non-shareable instead of
   * inner shareable.
   */
  // set attributes in S2CR
  ret = pulp_smmu_set_attr(pulp_ptr);
  if (ret) {
    return ret;
  }

  // read and set attributes in MAIR0
  offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_MAIR0_OFFSET_B;
  value = ioread32((void *)((unsigned long)pulp->smmu + offset));
  iowrite32(value, (void *)((unsigned long)pulp->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to MAIR0 of CB %i\n", value, cbndx);

  // set same attributes in MAIR1
  offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_MAIR1_OFFSET_B;
  iowrite32(value, (void *)((unsigned long)pulp->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to MAIR1 of CB %i\n", value, cbndx);

  // configure CBAR
  offset = SMMU_CBAR_OFFSET_B + cbndx*4;
  value = ioread32((void *)((unsigned long)pulp->smmu + offset));

  // set shareability - reserved: 0b00, outer shareable: 0b01, inner shareable: 0b10, non-shareable: 0b11
  BF_SET(value, 0b11, SMMU_CBAR_BPSHCFG, 2);
  iowrite32(value, (void *)((unsigned long)pulp->smmu + offset));
  if (DEBUG_LEVEL_SMMU > 2)
    printk(KERN_INFO "PULP - SMMU: Writing %#x to CBAR%i\n", value, cbndx);

  /*
   * enable RAB for bypassing
   */
  rab_slice_req.date_cur       = RAB_MAX_DATE_MH;
  rab_slice_req.date_exp       = RAB_MAX_DATE_MH;
  rab_slice_req.page_ptr_idx   = 0;
  rab_slice_req.page_idx_start = 0;
  rab_slice_req.page_idx_end   = 0;
  rab_slice_req.rab_port       = 1;
  rab_slice_req.rab_slice      = 0;
  rab_slice_req.flags_drv      = 0;
  BIT_SET(rab_slice_req.flags_drv, RAB_FLAGS_DRV_CONST | RAB_FLAGS_DRV_EVERY);
  rab_slice_req.flags_hw       = 0;
  BIT_SET(rab_slice_req.flags_hw, RAB_FLAGS_HW_WRITE | RAB_FLAGS_HW_READ | RAB_FLAGS_HW_EN);
  if (coherent)
    BIT_SET(rab_slice_req.flags_hw, RAB_FLAGS_HW_CC);

  // set to bypassing
  for (i=0; i<2; i++) {
    // two segments 0 - L3, L3 - end
    if (i == 0) {
      rab_slice_req.addr_start    = 0;
      rab_slice_req.addr_end      = L3_MEM_BASE_ADDR;
      rab_slice_req.addr_offset   = 0;
    }
    else {// (i == 1)
      rab_slice_req.addr_start    = L3_MEM_BASE_ADDR + L3_MEM_SIZE_B;
      rab_slice_req.addr_end      = 0xFFFFFFFF;
      rab_slice_req.addr_offset   = L3_MEM_BASE_ADDR + L3_MEM_SIZE_B;
    }

    for (j=0; j<RAB_L1_N_MAPPINGS_PORT_1; j++) {
      rab_slice_req.rab_mapping = j;

      //  check for free field in page_ptrs list
      ret = pulp_rab_page_ptrs_get_field(&rab_slice_req);
      if (ret) {
        return ret;
      }

      // get a free slice
      ret = pulp_rab_slice_get(&rab_slice_req);
      if (ret) {
        return ret;
      }

      // free memory of slices to be re-configured
      pulp_rab_slice_free(pulp_ptr->rab_config, &rab_slice_req);

      // setup slice
      ret = pulp_rab_slice_setup(pulp_ptr->rab_config, &rab_slice_req, NULL);
      if (ret) {
        return ret;
      }
    }
  }

  if (DEBUG_LEVEL_SMMU > 2) {
    pulp_rab_mapping_print(pulp_ptr->rab_config, 0);
  }

  printk(KERN_INFO "PULP - SMMU: Enabled.\n");

  return 0;
}

/**
 * Disable SMMU.
 *
 * This function disables for virtual-to-physical address translation for PULP by the SMMU.
 * It disables the RAB bypassing and enables SMMU bypassing.
 *
 * @param   pulp_ptr Pointer to PulpDev structure.
 *
 * @return  0 on success, a nonzero errno on errros.
 */
int pulp_smmu_dis(PulpDev *pulp_ptr)
{
  int ret, i;

  /*
   * disable RAB bypassing
   */
  ret = pulp_rab_release();
  if (ret) {
    printk(KERN_WARNING "PULP - SMMU: Failed to disable RAB bypassing.\n");
    return ret;
  }

  /*
   * distroy worker thread
   */
  if (smmu_fh_wq) {
    // Flush and destroy the workqueue, and reset workqueue pointer to default value.
    destroy_workqueue(smmu_fh_wq);

    printk(KERN_INFO "PULP - SMMU: Fault handling worker thread disabled.\n");
  }

  /*
   * unmap shared pages from I/O page table
   */
  for (i=N_PAGES-1; i>=0; i--) {
    if (iova_array[i] > 0) {
      if (DEBUG_LEVEL_SMMU_FH > 0)
        printk(KERN_INFO "PULP - SMMU: iova_array[%i] = %#lx\n", i, iova_array[i]);

      // unmap the page
      iommu_unmap(smmu_domain_ptr, iova_array[i], size);

      iova_array[i] = 0;

      // cache invalidation (in case of prefetching/speculation...)
      if (!coherent)
        pulp_mem_cache_inv(pages_ptrs[i], 0, PAGE_SIZE);

      // unpin user-space memory
      if ( !PageReserved(pages_ptrs[i]) )
        SetPageDirty(pages_ptrs[i]);
      put_page(pages_ptrs[i]);
    }
  }
  page_idx = 0;

  /*
   * disable smmu_domain
   */
  // detach the domain from the device
  iommu_detach_device(smmu_domain_ptr, pulp_ptr->dt_dev_ptr);

  // free the domain
  iommu_domain_free(smmu_domain_ptr);

  // re-set SMMU attribues
  ret = pulp_smmu_set_attr(pulp_ptr);
  if (ret) {
    return ret;
  }

  // re-enable SMMU bypass
  ret = pulp_smmu_bypass(pulp_ptr);
  if (ret) {
    return ret;
  }

  printk(KERN_INFO "PULP - SMMU: Disabled.\n");

  return 0;
}
//}}}

// fault {{{
/***********************************************************************************
 *
 * ███████╗ █████╗ ██╗   ██╗██╗  ████████╗
 * ██╔════╝██╔══██╗██║   ██║██║  ╚══██╔══╝
 * █████╗  ███████║██║   ██║██║     ██║
 * ██╔══╝  ██╔══██║██║   ██║██║     ██║
 * ██║     ██║  ██║╚██████╔╝███████╗██║
 * ╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝
 *
 ***********************************************************************************/

/**
 * Schedule the SMMU bottom-half fault handler.
 *
 * This function is the top-half SMMU fault handler registered to the Linux IOMMU API.
 * On a translation fault, it is called in interrupt context and then schedules the
 * bottom half in process context.
 *
 * @return  0 on success.
 */
int pulp_smmu_fh_sched(struct iommu_domain *smmu_domain_ptr, struct device *pulp_dev_ptr,
                       unsigned long iova, int flags, void * smmu_token_ptr)
{
  int ret = 0;
  unsigned int offset;
  unsigned int value = sctlr;

  if (DEBUG_LEVEL_SMMU_FH > 1)
    printk(KERN_INFO "PULP - SMMU: Handling fault. iova = %#lx, flags = %i.\n", iova, flags);

  // prepare the job - make sure it is safe to modify iova_faulty
  spin_lock(&smmu_fault_lock);
  while ( smmu_fault_status != READY ) {
    spin_unlock(&smmu_fault_lock);

    // busy waiting
    udelay(10);

    spin_lock(&smmu_fault_lock);
  }
  // pass iova
  iova_faulty       = iova;
  smmu_fault_status = WAIT;
  spin_unlock(&smmu_fault_lock);

  // schedule the job
  queue_work(smmu_fh_wq, &smmu_fh_w);

  // disable context fault interrupts
  // The interrupt is asserted until the SS bit in FSR is cleared. This only happens when resuming
  // or terminating the faulting transaction (performed by bottom half).
  offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_SCTLR_OFFSET_B;
  BIT_CLEAR(value, SMMU_SCTLR_CFIE);
  iowrite32(value, (void *)((unsigned long)pulp->smmu + offset));

  return ret;
}

/**
 * Handle SMMU translation faults.
 *
 * This function is the bottom-half SMMU fault handler scheduled in process context by the top
 * half. It handles translation faults by pinning the requested user-space pages an mapping them
 * to the IOMMU context or I/O virtual address space.
 *
 * @return  0 on success.
 */
void pulp_smmu_handle_fault(void)
{
  int ret = 0;
  unsigned long vaddr, offset, flags, iova;
  unsigned fsr, value;
  phys_addr_t paddr;
  size_t size;
  int prot;

  int write = 1;
  size = PAGE_SIZE;

  // read fsr for later clearance
  offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_FSR_OFFSET_B;
  fsr = ioread32((void *)((unsigned long)pulp->smmu + offset));

  // sync with fault handler (interrupt context)
  spin_lock_irqsave(&smmu_fault_lock, flags);
  iova              = iova_faulty;
  smmu_fault_status = READY;
  spin_unlock_irqrestore(&smmu_fault_lock, flags); // release the spinlock

  value = BF_GET(fsr, 2, 7);
  if ( (value != 0) || (BF_GET(fsr, SMMU_CB_FSR_TF, 1) == 0) ) {
    printk(KERN_WARNING "PULP - SMMU: Can only handle translation faults but got FSR %#x on address %#lx.\n",fsr , iova);
    ret = -EFAULT;
    goto pulp_smmu_handle_fault_error;
  }

  // align address to page border / 4kB
  vaddr  = (unsigned long)(iova) & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned long)*8-PAGE_SHIFT);

  if (DEBUG_LEVEL_SMMU_FH > 0)
    printk(KERN_INFO "PULP - SMMU: Faulty address = %#lx, request user-space address = %#lx\n",
      iova, vaddr);

  // get pointer to user-space buffer and lock it into memory, get a single page
  down_read(&user_task->mm->mmap_sem);
  ret = get_user_pages_remote(user_task, user_task->mm, vaddr, 1, write ? FOLL_WRITE : 0, &pages_ptrs[page_idx], NULL);
  up_read(&user_task->mm->mmap_sem);
  if ( ret != 1 ) {
    printk(KERN_WARNING "PULP - SMMU: Could not get requested user-space virtual address %#lx.\n", iova);
    ret = -EFAULT;
    goto pulp_smmu_handle_fault_error;
  }

  // virtual-to-physical address translation
  paddr = (phys_addr_t)page_to_phys(pages_ptrs[page_idx]);

  if (DEBUG_LEVEL_SMMU_FH > 0)
    printk(KERN_INFO "PULP - SMMU: Physical address = %#lx\n",(long unsigned)paddr);

  iova_array[page_idx] = vaddr;

  // flush data caches
  if (!coherent)
    pulp_mem_cache_flush(pages_ptrs[page_idx], 0, size);

  page_idx++;

  // prepare mapping
  prot = IOMMU_READ | (write ? IOMMU_WRITE : 0) | (coherent ? IOMMU_CACHE : 0);

  // map it
  ret = iommu_map(smmu_domain_ptr, vaddr, paddr, size, prot);
  if (ret) {
    printk(KERN_WARNING "PULP - SMMU: Could not map %#lx to SMMU, ERROR = %i.\n", vaddr, ret);
    goto pulp_smmu_handle_fault_error;
  }

  // sync with fault handler (interrupt context)
  pulp_smmu_handle_fault_error:

    // clear FSR
    offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_FSR_OFFSET_B;
    value = fsr;
    iowrite32(value, (void *)((unsigned long)pulp->smmu + offset));

    // resume or terminate transaction
    offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_RESUME_OFFSET_B;
    if (ret)
      value = 1; // terminate
    else
      value = 0; // retry
    iowrite32(value, (void *)((unsigned long)pulp->smmu + offset));

    // re-enable context fault interrupts
    offset = SMMU_CB_OFFSET_B + cbndx*SMMU_CB_SIZE_B + SMMU_CB_SCTLR_OFFSET_B;
    iowrite32(sctlr, (void *)((unsigned long)pulp->smmu + offset));

  return;
}
//}}}
