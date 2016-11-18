#include "pulp_rab.h"
#include "pulp_mem.h" /* for cache invalidation */

// global variables {{{
static L1Tlb l1;
static L2Tlb l2;
static PulpDev * pulp;

// striping information
static RabStripeReq rab_stripe_req[RAB_L1_N_MAPPINGS_PORT_1];

// miss handling routine
static char rab_mh_wq_name[10] = "RAB_MH_WQ";
static struct workqueue_struct *rab_mh_wq;
static struct work_struct rab_mh_w;
static struct task_struct *user_task;
static struct mm_struct *user_mm;
static unsigned rab_mh_addr[RAB_MH_FIFO_DEPTH];
static unsigned rab_mh_id[RAB_MH_FIFO_DEPTH];
static unsigned rab_mh;
static unsigned rab_mh_date;
static unsigned rab_mh_acp;
static unsigned rab_mh_lvl;

// AX logger 
#if PLATFORM == JUNO
  static unsigned * rab_ar_log_buf;
  static unsigned * rab_aw_log_buf;
  static unsigned rab_ar_log_buf_idx = 0;
  static unsigned rab_aw_log_buf_idx = 0;
#endif

// for profiling
#if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
  static unsigned arm_clk_cntr_value = 0;
  static unsigned arm_clk_cntr_value_start = 0;

  static unsigned n_cyc_response       = 0; // per update request
  static unsigned n_cyc_update         = 0; // per update request
  static unsigned n_cyc_setup          = 0; // per stripe request (multiple elements, pages)
  static unsigned n_cyc_cache_flush    = 0; // per striped element
  static unsigned n_cyc_get_user_pages = 0; // per striped element
  
  static unsigned * n_cyc_buf_response;
  static unsigned * n_cyc_buf_update;
  static unsigned * n_cyc_buf_setup;
  static unsigned * n_cyc_buf_cache_flush;
  static unsigned * n_cyc_buf_get_user_pages;

  static unsigned idx_buf_response;
  static unsigned idx_buf_update;
  static unsigned idx_buf_setup;
  static unsigned idx_buf_cache_flush;
  static unsigned idx_buf_get_user_pages;
  
  static unsigned n_cyc_tot_response;
  static unsigned n_cyc_tot_update;
  static unsigned n_cyc_tot_setup;
  static unsigned n_cyc_tot_cache_flush;
  static unsigned n_cyc_tot_get_user_pages;

  #ifdef PROFILE_RAB_STR
    static unsigned n_cyc_cleanup      = 0; // per stripe free    (multiple elements, pages)
    static unsigned n_cyc_map_sg       = 0; // per striped element
  
    static unsigned * n_cyc_buf_cleanup;
    static unsigned * n_cyc_buf_map_sg;
  
    static unsigned idx_buf_cleanup;
    static unsigned idx_buf_map_sg;
  
    static unsigned n_cyc_tot_cleanup;
    static unsigned n_cyc_tot_map_sg;

    static unsigned n_pages_setup;
    static unsigned n_cleanups;
    static unsigned n_slices_updated;
    static unsigned n_updates;
  #endif
  
  #ifdef PROFILE_RAB_MH
    static unsigned n_cyc_response_tmp[N_CORES_TIMER];
    
    static unsigned n_cyc_schedule = 0; // per miss

    static unsigned * n_cyc_buf_schedule;
    
    static unsigned idx_buf_schedule;
    
    static unsigned n_cyc_tot_schedule;
  
    static unsigned n_first_misses = 0;
    static unsigned n_misses       = 0;
  #endif

#endif

// }}}

// functions

// init {{{
/**
 * Initialize the RAB.
 *
 * @pulp: ptr to PulpDev structure, used to initalize local copy.
 */
int pulp_rab_init(PulpDev * pulp_ptr)
{
  int err = 0;

  // initialize the pointer for pulp_rab_update/switch
  pulp = pulp_ptr;

  // initialize management structures and L2 hardware
  pulp_rab_l1_init();
  #if PLATFORM != ZEDBOARD
    pulp_rab_l2_init(pulp->rab_config);
  #endif

  // initialize miss handling control
  rab_mh = 0;
  rab_mh_date = 0;
  rab_mh_acp = 0;
  rab_mh_lvl = 0;

  // initialize AX logger
  #if PLATFORM == JUNO
    err = pulp_rab_ax_log_init();
    if (err)
      return err;
  #endif

  // prepare for profiling
  #if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
    err = pulp_rab_prof_init();
    if (err)
      return err;
  #endif 

  return err;
}
// }}}

// L1 Management {{{
/***********************************************************************************
 *
 * ██╗     ██╗    ███╗   ███╗ ██████╗ ███╗   ███╗████████╗
 * ██║    ███║    ████╗ ████║██╔════╝ ████╗ ████║╚══██╔══╝
 * ██║    ╚██║    ██╔████╔██║██║  ███╗██╔████╔██║   ██║   
 * ██║     ██║    ██║╚██╔╝██║██║   ██║██║╚██╔╝██║   ██║   
 * ███████╗██║    ██║ ╚═╝ ██║╚██████╔╝██║ ╚═╝ ██║   ██║   
 * ╚══════╝╚═╝    ╚═╝     ╚═╝ ╚═════╝ ╚═╝     ╚═╝   ╚═╝   
 * 
 ***********************************************************************************/                                                       

// l1_init {{{
/**
 * Initialize the arrays the driver uses to manage the RAB.
 */
void pulp_rab_l1_init(void)
{
  int i,j;

  // Port 0
  for (i=0; i<RAB_L1_N_SLICES_PORT_0; i++) {
    l1.port_0.slices[i].date_cur       = 0;
    l1.port_0.slices[i].date_exp       = 0;
    l1.port_0.slices[i].addr_start     = 0;
    l1.port_0.slices[i].addr_end       = 0;
    l1.port_0.slices[i].addr_offset    = 0;
    l1.port_0.slices[i].flags_hw       = 0;
  }

  // Port 1
  for (i=0; i<RAB_L1_N_MAPPINGS_PORT_1; i++) {
    for (j=0; j<RAB_L1_N_SLICES_PORT_1; j++) {
      l1.port_1.mappings[i].slices[j].date_cur       = 0;
      l1.port_1.mappings[i].slices[j].date_exp       = 0;
      l1.port_1.mappings[i].slices[j].page_ptr_idx   = 0;
      l1.port_1.mappings[i].slices[j].page_idx_start = 0;
      l1.port_1.mappings[i].slices[j].page_idx_end   = 0;
      l1.port_1.mappings[i].slices[j].flags_drv      = 0;
      l1.port_1.mappings[i].slices[j].addr_start     = 0;
      l1.port_1.mappings[i].slices[j].addr_end       = 0;
      l1.port_1.mappings[i].slices[j].addr_offset    = 0;
      l1.port_1.mappings[i].slices[j].flags_hw       = 0;
      l1.port_1.mappings[i].page_ptrs[j]             = 0;
      l1.port_1.mappings[i].page_ptr_ref_cntrs[j]    = 0;
    }
  }
  l1.port_1.mapping_active = 0;

  return;
}
// }}}

// page_ptrs_get_field {{{
/**
 * Get a free field from the page_ptrs array.
 *
 * @rab_slice_req: the obtained field is stored in
 * rab_slice_req->page_ptr_idx.
 */
int pulp_rab_page_ptrs_get_field(RabSliceReq *rab_slice_req)
{
  int err,i;

  err = 0;
 
  // get field in page_ptrs array
  for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
    if ( l1.port_1.mappings[rab_slice_req->rab_mapping].page_ptr_ref_cntrs[i] == 0 ) {
      rab_slice_req->page_ptr_idx = i;
      break;
    }    
    if ( i == (RAB_L1_N_SLICES_PORT_1-1) ) {
      printk(KERN_INFO "PULP - RAB L1: No page_ptrs field available on Port 1.\n");
      return -EIO;
    }
  }

  return err;
}
// }}}

// slice_check {{{
/**
 * Check whether a particular slice has expired. Returns 1 if slice
 * has expired, 0 otherwise.
 *
 * @rab_slice_req: specifies the slice to check as well as the current
 *                 date.
 */
int pulp_rab_slice_check(RabSliceReq *rab_slice_req)
{
  int expired;
  unsigned char date_exp_i;
  
  expired = 0;
  
  if (DEBUG_LEVEL_RAB > 2) {
    printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Testing.\n",
      rab_slice_req->rab_port, rab_slice_req->rab_mapping, rab_slice_req->rab_slice);
  }

  if (rab_slice_req->rab_port == 0) {
    date_exp_i = l1.port_0.slices[rab_slice_req->rab_slice].date_exp;
  }
  else { // Port 1
    date_exp_i = l1.port_1.mappings[rab_slice_req->rab_mapping].slices[rab_slice_req->rab_slice].date_exp;
  }

  if ( (rab_slice_req->date_cur > date_exp_i) || // found an expired slice
       ((rab_slice_req->date_cur == 0) && ( RAB_MAX_DATE_MH == date_exp_i)) ) // wrap around in RAB miss handling mode
    expired = 1;
  
  return expired;
}
// }}}

// slice_get {{{
/**
 * Get a free RAB slice. Returns 0 on success, 1 otherwise.
 *
 * @rab_slice_req: specifies the current date.
 */
int pulp_rab_slice_get(RabSliceReq *rab_slice_req)
{
  int err, i;
  unsigned n_slices;

  err = 0;

  if (rab_slice_req->rab_port == 0)
    n_slices = RAB_L1_N_SLICES_PORT_0;
  else // Port 1
    n_slices = RAB_L1_N_SLICES_PORT_1;
  
  // get slice number
  for (i=0; i<n_slices; i++) {
    rab_slice_req->rab_slice = i;   

    if ( pulp_rab_slice_check(rab_slice_req) ) // found an expired slice
      break;
    else if (i == (n_slices-1) ) { // no slice free
      err = 1;
      printk(KERN_INFO "PULP RAB L1: No slice available on Port %d.\n", rab_slice_req->rab_port);
    }
  }

  return err;
}
// }}}

// slice_free {{{
/**
 * Free a RAB slice and unlock any corresponding memory pages.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @rab_slice_req: specifies the slice to free.
 */
void pulp_rab_slice_free(void *rab_config, RabSliceReq *rab_slice_req)
{
  int i;
  unsigned char page_ptr_idx_old, page_idx_start_old, page_idx_end_old;
  unsigned char port, flags_drv;
  unsigned mapping, slice, entry;

  struct page ** pages_old;

  port      = rab_slice_req->rab_port;
  mapping   = rab_slice_req->rab_mapping;
  slice     = rab_slice_req->rab_slice;
  flags_drv = rab_slice_req->flags_drv;
 
  if (DEBUG_LEVEL_RAB > 0) {
    printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Freeing.\n",
      port, mapping, slice);
  }

  // get old configuration of the selected slice
  if (port == 0) {
    page_ptr_idx_old = 0;
    pages_old        = 0;
  }
  else { // port 1
    page_ptr_idx_old = l1.port_1.mappings[mapping].slices[slice].page_ptr_idx;
    pages_old        = l1.port_1.mappings[mapping].page_ptrs[page_ptr_idx_old];
  }

  if (pages_old) { // not used for a constant mapping, Port 0 can only hold constant mappings
    
    // deactivate the slice
    entry = 0x20*(port*RAB_L1_N_SLICES_PORT_0+slice);
    if ( l1.port_1.mapping_active == mapping )
      iowrite32(0, (void *)((unsigned long)rab_config+entry+0x38));

    // determine pages to be unlocked
    page_idx_start_old = l1.port_1.mappings[mapping].slices[slice].page_idx_start;
    page_idx_end_old   = l1.port_1.mappings[mapping].slices[slice].page_idx_end;

    // unlock remapped pages and invalidate caches
    if ( !(flags_drv & 0x2) &&   // do not unlock pages in striped mode until the last slice is freed
         !(flags_drv & 0x4) ) {  // only unlock pages in multi-mapping rule when the last mapping is removed 
      for (i=page_idx_start_old;i<=page_idx_end_old;i++) {
        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Unlocking Page %d.\n",
            port, mapping, slice, i);
        }
        //// invalidate caches --- invalidates entire pages only --- really needed?
        //if ( !(flags_hw & 0x8) )
        //  pulp_mem_cache_inv(pages_old[i],0,PAGE_SIZE);
        // unlock
        if ( !PageReserved(pages_old[i]) ) 
          SetPageDirty(pages_old[i]);
        page_cache_release(pages_old[i]);
      }
    }
    // lower reference counter
    l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old]--;
      
    // free memory if no more references exist
    if ( !l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old] ) {
      if ( !(flags_drv & 0x4) ) // only free pages ptr in multi-mapping rule when the last mapping is removed 
        kfree(pages_old);
      l1.port_1.mappings[mapping].page_ptrs[page_ptr_idx_old] = 0;
    }
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB L1: Number of references to pages pointer = %d.\n", 
        l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old]);
    }
  }
  else if ( (port == 1) && (flags_drv & 0x1) ) {
    if (l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old]) 
      l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old]--;
    if (mapping == l1.port_1.mapping_active) { // also free constant, active mappings on Port 1
      entry = 0x20*(port*RAB_L1_N_SLICES_PORT_0+slice);
      iowrite32(0, (void *)((unsigned long)rab_config+entry+0x38));
    }
  }

  // clear entries in management structs
  if (port == 0) {
    l1.port_0.slices[slice].date_cur       = 0;
    l1.port_0.slices[slice].date_exp       = 0;
    l1.port_0.slices[slice].addr_start     = 0;
    l1.port_0.slices[slice].addr_end       = 0;
    l1.port_0.slices[slice].addr_offset    = 0;
    l1.port_0.slices[slice].flags_hw       = 0;
  }
  else { // port 1
    l1.port_1.mappings[mapping].slices[slice].date_cur       = 0;
    l1.port_1.mappings[mapping].slices[slice].date_exp       = 0;
    l1.port_1.mappings[mapping].slices[slice].page_ptr_idx   = 0;
    l1.port_1.mappings[mapping].slices[slice].page_idx_start = 0;
    l1.port_1.mappings[mapping].slices[slice].page_idx_end   = 0;
    l1.port_1.mappings[mapping].slices[slice].flags_drv      = 0;
    l1.port_1.mappings[mapping].slices[slice].addr_start     = 0;
    l1.port_1.mappings[mapping].slices[slice].addr_end       = 0;
    l1.port_1.mappings[mapping].slices[slice].addr_offset    = 0;
    l1.port_1.mappings[mapping].slices[slice].flags_hw       = 0;
  }

  return;
}
// }}}

// slice_setup {{{
/**
 * Setup a RAB slice: 1) setup the drivers' managment arrays, 2)
 * configure the hardware using iowrite32(). Returns 0 on success,
 * -EIO otherwise.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @rab_slice_req: specifies the slice to setup.
 * @pages: pointer to the page structs of the remapped user-space memory pages.
 */
int pulp_rab_slice_setup(void *rab_config, RabSliceReq *rab_slice_req, struct page **pages)
{
  unsigned offset, port, mapping, slice;

  port    = rab_slice_req->rab_port;
  mapping = rab_slice_req->rab_mapping;
  slice   = rab_slice_req->rab_slice;
 
  // occupy the slice
  if (port == 0) {
    l1.port_0.slices[slice].date_cur = rab_slice_req->date_cur;
    l1.port_0.slices[slice].date_exp = rab_slice_req->date_exp;
    
    l1.port_0.slices[slice].addr_start  = rab_slice_req->addr_start;
    l1.port_0.slices[slice].addr_end    = rab_slice_req->addr_end;
    l1.port_0.slices[slice].addr_offset = rab_slice_req->addr_offset;
    l1.port_0.slices[slice].flags_hw    = rab_slice_req->flags_hw;
  }
  else { // Port 1
    l1.port_1.mappings[mapping].slices[slice].date_cur       = rab_slice_req->date_cur;
    l1.port_1.mappings[mapping].slices[slice].date_exp       = rab_slice_req->date_exp;
    l1.port_1.mappings[mapping].slices[slice].page_ptr_idx   = rab_slice_req->page_ptr_idx;
    l1.port_1.mappings[mapping].slices[slice].page_idx_start = rab_slice_req->page_idx_start;
    l1.port_1.mappings[mapping].slices[slice].page_idx_end   = rab_slice_req->page_idx_end;
    l1.port_1.mappings[mapping].slices[slice].flags_drv      = rab_slice_req->flags_drv;

    l1.port_1.mappings[mapping].slices[slice].addr_start  = rab_slice_req->addr_start;
    l1.port_1.mappings[mapping].slices[slice].addr_end    = rab_slice_req->addr_end;
    l1.port_1.mappings[mapping].slices[slice].addr_offset = rab_slice_req->addr_offset;
    l1.port_1.mappings[mapping].slices[slice].flags_hw    = rab_slice_req->flags_hw;
  }

  if (port == 0) {
    // no page ptrs maintained
  }
  else { // Port 1

    // for the first segment, check that the selected reference list
    // entry is really free = memory has properly been freed
    if ( l1.port_1.mappings[mapping].page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx]
         & !rab_slice_req->page_idx_start & !(rab_slice_req->flags_drv & 0x2) ) { 
      printk(KERN_WARNING "PULP - RAB L1: Selected reference list entry not free. Number of references = %d.\n", 
        l1.port_1.mappings[mapping].page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx]);
      return -EIO;
    }
  
    l1.port_1.mappings[mapping].page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx]++;
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB L1: Number of references to pages pointer = %d.\n",
        l1.port_1.mappings[mapping].page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx]);
    }

    if (rab_slice_req->flags_drv & 0x1)
      l1.port_1.mappings[mapping].page_ptrs[rab_slice_req->page_ptr_idx] = 0;
    else
      l1.port_1.mappings[mapping].page_ptrs[rab_slice_req->page_ptr_idx] = pages;
  }
  
  if (DEBUG_LEVEL_RAB > 0)
    printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Setting up.\n", port, mapping, slice);

  // set up new slice, configure the hardware
  if ( (port == 0) || (l1.port_1.mapping_active == mapping) ) {
    offset = 0x20*(port*RAB_L1_N_SLICES_PORT_0+slice);
  
    iowrite32(rab_slice_req->addr_start,  (void *)((unsigned long)rab_config+offset+0x20));
    iowrite32(rab_slice_req->addr_end,    (void *)((unsigned long)rab_config+offset+0x28));
    IOWRITE_L(rab_slice_req->addr_offset, (void *)((unsigned long)rab_config+offset+0x30));
    iowrite32(rab_slice_req->flags_hw,    (void *)((unsigned long)rab_config+offset+0x38));
 
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP - RAB L1: addr_start  %#x\n",  rab_slice_req->addr_start );
      printk(KERN_INFO "PULP - RAB L1: addr_end    %#x\n",  rab_slice_req->addr_end   );
      printk(KERN_INFO "PULP - RAB L1: addr_offset %#lx\n", rab_slice_req->addr_offset);
      printk(KERN_INFO "PULP - RAB L1: flags_hw    %#x\n",  rab_slice_req->flags_hw   );
    }
  }  

  return 0;  
}
// }}}

// num_free_slices {{{
/**
 * Get the number of free RAB slices.
 *
 * @rab_slice_req: specifies the current date.
 */
int pulp_rab_num_free_slices(RabSliceReq *rab_slice_req)
{
  int num_free_slice, i;
  unsigned n_slices;

  num_free_slice = 0;

  if (rab_slice_req->rab_port == 0)
    n_slices = RAB_L1_N_SLICES_PORT_0;
  else // Port 1
    n_slices = RAB_L1_N_SLICES_PORT_1;
  
  for (i=0; i<n_slices; i++) {
    rab_slice_req->rab_slice = i;   

    if ( pulp_rab_slice_check(rab_slice_req) ) // found an expired slice
      num_free_slice++;
  }

  return num_free_slice;
}
// }}}

// mapping_get_active {{{
/**
 * Get the index of the currently active RAB mapping on Port 1.
 */
int pulp_rab_mapping_get_active()
{
  return (int)l1.port_1.mapping_active;
}
// }}}

// mappping_switch {{{
/**
 * Switch the current RAB setup with another mapping. In case a mapping 
 * contains a striped config, Stripe 0 will be set up.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @rab_mapping: specifies the mapping to set up the RAB with.
 */
void pulp_rab_mapping_switch(void *rab_config, unsigned rab_mapping)
{
  int i;
  unsigned offset;
  unsigned char prot;

  unsigned mapping_active; 

  mapping_active = l1.port_1.mapping_active;

  if (DEBUG_LEVEL_RAB > 0)
    printk(KERN_INFO "PULP - RAB L1: Switch from Mapping %d to %d.\n",mapping_active,rab_mapping);

  // de-activate old mapping
  for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
    RAB_GET_PROT(prot, l1.port_1.mappings[mapping_active].slices[i].flags_hw);
    if (prot) { // de-activate slices with old active config

      offset = 0x20*(1*RAB_L1_N_SLICES_PORT_0+i);
      iowrite32(0x0,(void *)((unsigned long)rab_config+offset+0x38));

      if (DEBUG_LEVEL_RAB > 0)
        printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Disabling.\n",1,mapping_active,i);
    }
  }

  // set up new mapping
  for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
    RAB_GET_PROT(prot, l1.port_1.mappings[rab_mapping].slices[i].flags_hw);
    if (prot & 0x1) { // activate slices with new active config

      offset = 0x20*(1*RAB_L1_N_SLICES_PORT_0+i);
  
      iowrite32(l1.port_1.mappings[rab_mapping].slices[i].addr_start,  (void *)((unsigned long)rab_config+offset+0x20));
      iowrite32(l1.port_1.mappings[rab_mapping].slices[i].addr_end,    (void *)((unsigned long)rab_config+offset+0x28));
      IOWRITE_L(l1.port_1.mappings[rab_mapping].slices[i].addr_offset, (void *)((unsigned long)rab_config+offset+0x30));
      iowrite32(l1.port_1.mappings[rab_mapping].slices[i].flags_hw,    (void *)((unsigned long)rab_config+offset+0x38));

      if (DEBUG_LEVEL_RAB > 0)
        printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Setting up.\n",1,rab_mapping,i);
      if (DEBUG_LEVEL_RAB > 1) {
        printk(KERN_INFO "PULP - RAB L1: addr_start  %#x\n",  l1.port_1.mappings[rab_mapping].slices[i].addr_start );
        printk(KERN_INFO "PULP - RAB L1: addr_end    %#x\n",  l1.port_1.mappings[rab_mapping].slices[i].addr_end   );
        printk(KERN_INFO "PULP - RAB L1: addr_offset %#lx\n", l1.port_1.mappings[rab_mapping].slices[i].addr_offset);
      }
    }
  }

  l1.port_1.mapping_active = rab_mapping;

  return;
}
// }}}

// mapping_print {{{
/**
 * Print RAB mappings.
 *
 * @rab_config:  kernel virtual address of the RAB configuration port.
 * @rab_mapping: specifies the mapping to to print, 0xAAAA for actual
 *               RAB configuration, 0xFFFF for all mappings.
 */
void pulp_rab_mapping_print(void *rab_config, unsigned rab_mapping)
{
  int mapping_min, mapping_max;
  int i,j;
  unsigned offset, prot, flags_hw, n_slices, page_ptr_idx;

  if (rab_mapping == 0xFFFF) {
    mapping_min = 0;
    mapping_max = RAB_L1_N_MAPPINGS_PORT_1;
  }
  else if (rab_mapping == 0xAAAA) {
    mapping_min = 0;
    mapping_max = 0;
  }
  else {
    mapping_min = rab_mapping;
    mapping_max = rab_mapping+1;
  }

  for (j=mapping_min; j<mapping_max; j++) {
    printk(KERN_INFO "PULP - RAB L1: Printing Mapping %d: \n",j);

    for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
      RAB_GET_PROT(prot, l1.port_1.mappings[rab_mapping].slices[i].flags_hw);
      if (prot) {
        printk(KERN_INFO "Port %d, Slice %2d: %#x - %#x -> %#lx , flags_hw = %#x\n", 1, i,
          l1.port_1.mappings[j].slices[i].addr_start,
          l1.port_1.mappings[j].slices[i].addr_end,
          l1.port_1.mappings[j].slices[i].addr_offset,
          l1.port_1.mappings[j].slices[i].flags_hw);
      }
    }
  } 

  if ( (rab_mapping ==  0xFFFF) || (rab_mapping == 0xAAAA) ) {
    printk(KERN_INFO "PULP - RAB L1: Printing active configuration: \n");
    
    for (j=0; j<2; j++) {
      if (j == 0) // Port 0
        n_slices = RAB_L1_N_SLICES_PORT_0;
      else // Port 1
        n_slices = RAB_L1_N_SLICES_PORT_1;

      for (i=0; i<n_slices; i++) {
        offset = 0x20*(j*RAB_L1_N_SLICES_PORT_0+i);

        flags_hw = ioread32((void *)((unsigned long)rab_config+offset+0x38));
        RAB_GET_PROT(prot, flags_hw);
        if (prot) {
          printk(KERN_INFO "Port %d, Slice %2d: %#x - %#x -> %#lx , flags_hw = %#x\n", j, i,
            ioread32((void *)((unsigned long)rab_config+offset+0x20)),
            ioread32((void *)((unsigned long)rab_config+offset+0x28)),
            (unsigned long)IOREAD_L((void *)((unsigned long)rab_config+offset+0x30)),
            flags_hw);  
        }
      }
    }
  } 

  // print the RAB pages and reference counter lists
  for (j=mapping_min; j<mapping_max; j++) {
    printk(KERN_INFO "PULP - RAB L1: Printing Mapping %d: \n",j);

    for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
      RAB_GET_PROT(prot, l1.port_1.mappings[rab_mapping].slices[i].flags_hw);
      if (prot) {
        page_ptr_idx = l1.port_1.mappings[j].slices[i].page_ptr_idx;
        printk(KERN_INFO "Port %d, Slice %2d: page_ptrs[%2d] = %#lx, page_ptr_ref_cntrs[%2d] = %d\n", 1, i,
          page_ptr_idx, (unsigned long)l1.port_1.mappings[j].page_ptrs[page_ptr_idx],
          page_ptr_idx,                l1.port_1.mappings[j].page_ptr_ref_cntrs[page_ptr_idx]);
      }
    }
  }

  return;
}
// }}}

// }}}

// L2 Management {{{
/***********************************************************************************
 *
 * ██╗     ██████╗     ███╗   ███╗ ██████╗ ███╗   ███╗████████╗
 * ██║     ╚════██╗    ████╗ ████║██╔════╝ ████╗ ████║╚══██╔══╝
 * ██║      █████╔╝    ██╔████╔██║██║  ███╗██╔████╔██║   ██║   
 * ██║     ██╔═══╝     ██║╚██╔╝██║██║   ██║██║╚██╔╝██║   ██║   
 * ███████╗███████╗    ██║ ╚═╝ ██║╚██████╔╝██║ ╚═╝ ██║   ██║   
 * ╚══════╝╚══════╝    ╚═╝     ╚═╝ ╚═════╝ ╚═╝     ╚═╝   ╚═╝   
 * 
 ***********************************************************************************/   

// l2_init {{{
/**
 * Initialise L2 TLB HW and struct to zero.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 */
void pulp_rab_l2_init(void *rab_config)
{
  unsigned int i_port, i_set, i_entry, offset;
  for (i_port=0; i_port<1; i_port++) {
    for (i_set=0; i_set<RAB_L2_N_SETS; i_set++) {
      for (i_entry=0; i_entry<RAB_L2_N_ENTRIES_PER_SET; i_entry++) {
        // Clear VA ram. No need to clear PA ram.
        offset = ((i_port+1)*0x4000) + (i_set*RAB_L2_N_ENTRIES_PER_SET*4) + (i_entry*4);
        iowrite32( 0, (void *)((unsigned long)rab_config + offset)); 
        l2.set[i_set].entry[i_entry].flags = 0;
        l2.set[i_set].entry[i_entry].pfn_p = 0;
        l2.set[i_set].entry[i_entry].pfn_v = 0;
      }
      l2.set[i_set].next_entry_idx = 0;
      l2.set[i_set].is_full = 0;
    }
  }
  printk(KERN_INFO "PULP - RAB L2: Initialized VRAMs to 0.\n");

  return;
}
// }}}

// l2_setup_entry {{{
/**
 * Setup an L2TLB entry: 1) Check if a free spot is avaialble in L2 TLB,
 * 2) configure the HW using iowrite32(), 3) Configure kernel struct. 
 * Return 0 on success.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @tlb_entry: specifies the L2TLB entry to setup.
 * @port: RAB port
 * @enable_replace: an entry can be replaced if set is full.
 */
int pulp_rab_l2_setup_entry(void *rab_config, L2Entry *tlb_entry, char port, char enable_replace)
{
  unsigned set_num, entry_num;
  unsigned data_v, data_p, off_v, off_p;
  
  int err = 0;
  int full = 0;

  set_num = BF_GET(tlb_entry->pfn_v,0,5);
  entry_num = l2.set[set_num].next_entry_idx;

  //Check if set is full
  full = pulp_rab_l2_check_availability(tlb_entry, port);
  if (full == 1 && enable_replace == 0){
    err = 1;
    printk(KERN_WARNING "PULP - RAB L2: Port %d, Set %d: Full.\n", port, set_num);
  }
  if (full == 1 && enable_replace == 1){ 
    pulp_rab_l2_invalidate_entry(rab_config, port, set_num, entry_num);
    full = 0;
  }

  if (full == 0) {
    //Set is not full. Issue a write.
    off_v = (port+1)*0x4000 + set_num*RAB_L2_N_ENTRIES_PER_SET*4 + entry_num*4;
    
    data_v = tlb_entry->flags;
    BF_SET(data_v, tlb_entry->pfn_v, 4, 20); // Parameterise TODO.
    iowrite32(data_v, (void *)((unsigned long)rab_config+off_v));

    off_p = (port+1)*0x4000 + set_num*RAB_L2_N_ENTRIES_PER_SET*4 + entry_num*4 + 1024*4 ; // PA RAM address. Parameterise TODO.
    data_p =  tlb_entry->pfn_p;
    iowrite32(data_p, (void *)((unsigned long)rab_config+off_p));
    
    //printk("off_v = %#x, off_p = %#x \n",(unsigned)off_v, (unsigned)off_p);
    //printk("data_v = %#x, data_p = %#x \n",(unsigned)data_v, (unsigned)data_p);

    // Update kernel struct
    l2.set[set_num].entry[entry_num] = *tlb_entry;
    l2.set[set_num].next_entry_idx++;
    if (DEBUG_LEVEL_RAB > 0)
      printk(KERN_INFO "PULP - RAB L2: Port %d, Set %d, Entry %d: Setting up.\n", 
        port, set_num, entry_num);
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP - RAB L2: PFN_V= %#5x, PFN_P = %#5x\n", 
        tlb_entry->pfn_v, tlb_entry->pfn_p);
    }

    if (l2.set[set_num].next_entry_idx == RAB_L2_N_ENTRIES_PER_SET) {
      l2.set[set_num].is_full = 1;
      l2.set[set_num].next_entry_idx = 0;
    }
  }

  return err;
}
// }}}

// l2_check_availability {{{
/**
 * Check if a free spot is available in L2 TLB.
 * Return 0 if available. 1 if not available.
 * @tlb_entry: specifies the L2TLB entry to setup.
 * @port: RAB port
 */
int pulp_rab_l2_check_availability(L2Entry *tlb_entry, char port)
{
  unsigned set_num;
  int full = 0;

  set_num = BF_GET(tlb_entry->pfn_v,0,5);

  //Check if set is full
  if (l2.set[set_num].is_full == 1) {
    full = 1;
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB L2: Port %d, Set %d: Full.\n", port, set_num);
    }
    //return -EIO;
  }

  return full;
}
// }}}

// l2_invalidate_all_entries {{{
/**
 * Invalidate all valid L2 TLB entries. Keep the virtual and physical pageframe numbers intact.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @port: RAB port
 */
int pulp_rab_l2_invalidate_all_entries(void *rab_config, char port)
{
  int set_num, entry_num;

  for (set_num=0; set_num<RAB_L2_N_SETS; set_num++) {
    for (entry_num=0; entry_num<l2.set[set_num].next_entry_idx; entry_num++) {
      pulp_rab_l2_invalidate_entry(rab_config, port, set_num, entry_num);
    }
    l2.set[set_num].next_entry_idx = 0;
  }

  return 0;
}
// }}}

// l2_invalidate_entry {{{
/**
 * Invalidate one L2 TLB entry. Keep the virtual and physical pageframe numbers intact.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @port: RAB port
 * @set_num: Set number
 * @entry_num: Entry number in the set.
 */
int pulp_rab_l2_invalidate_entry(void *rab_config, char port, int set_num, int entry_num)
{
  unsigned data;
  struct page * page_old;

  if (DEBUG_LEVEL_RAB > 1) {
    printk(KERN_INFO "PULP - RAB L2: Port %d, Set %d, Entry %d: Freeing.\n", 
      port, set_num, entry_num);
  }
  data = l2.set[set_num].entry[entry_num].flags;
  data = data >> 1;
  data = data << 1; // LSB is now zero.
  l2.set[set_num].entry[entry_num].flags = data; // Update kernel struct
  BF_SET(data, l2.set[set_num].entry[entry_num].pfn_v, 4, 20); // Parameterise TODO.
  iowrite32( data, (void *)((unsigned long)rab_config+((port+1)*0x4000) + 
    (set_num*RAB_L2_N_ENTRIES_PER_SET*4) + (entry_num*4)) );

  // unlock pages and invalidate cache.
  page_old = l2.set[set_num].entry[entry_num].page_ptr;  
  if (!PageReserved(page_old)) 
    SetPageDirty(page_old);
  page_cache_release(page_old);  

  // free the allocated memory for page struct. TODO
  //kfree(l2.set[set_num].entry[entry_num].page_ptr);

  return 0;
}
// }}}

// l2_print_all_entries {{{
/**
 * Print all entries.
 *
 * @port: RAB port
 */
int pulp_rab_l2_print_all_entries(char port)
{
  int set_num, entry_num;
  printk(KERN_INFO "PULP - RAB L2: Printing config of Port %d\n",port);
  for (set_num=0; set_num<RAB_L2_N_SETS; set_num++) {
    for (entry_num=0; entry_num<l2.set[set_num].next_entry_idx; entry_num++) {
      printk(KERN_INFO "Set %d, Entry %d: PFN_V = %#x, PFN_P = %#x, Flags = %#x\n",
        set_num, entry_num, 
        l2.set[set_num].entry[entry_num].pfn_v,
        l2.set[set_num].entry[entry_num].pfn_p,
        l2.set[set_num].entry[entry_num].flags
        );
    }
  }
  return 0;
}
// }}}

// l2_print_valid_entries {{{
int pulp_rab_l2_print_valid_entries(char port)
{
  int set_num, entry_num;
  printk(KERN_INFO "PULP - RAB L2: Printing valid entries of Port %d\n",port);
  for (set_num=0; set_num<RAB_L2_N_SETS; set_num++) {
    for (entry_num=0; entry_num<l2.set[set_num].next_entry_idx; entry_num++) {
      if (l2.set[set_num].entry[entry_num].flags & 0x1) {
        printk(KERN_INFO "Set %d, Entry %d: PFN_V = %#x, PFN_P = %#x, Flags = %#x\n",
          set_num, entry_num,
          l2.set[set_num].entry[entry_num].pfn_v,
          l2.set[set_num].entry[entry_num].pfn_p,
          l2.set[set_num].entry[entry_num].flags
        );
      }
    }
  }
  return 0;
}
// }}}

//////// Other functions
// pulp_rab_l2_replace_entry()
// pulp_rab_l2_find_entry_to_replace()

// The user will give preference as L1 or L2. Default option is L1 . try to group all nearby L1 together and call ioctl, same as before.
// If all slices are used up, give back error for now. Later, we will make them fill L2 in case L1 is full.
// group all L2 together and call ioctl. There should be only one ioctl for all of L2.
// Further enhancement would be to use some algorithm to dynamically assign the mapping to L1 or L2 based on region size and frequency of occurance.

// }}}

// IOCTL Requests {{{
/***********************************************************************************
 *
 * ██╗ ██████╗  ██████╗████████╗██╗         ██████╗ ███████╗ ██████╗ 
 * ██║██╔═══██╗██╔════╝╚══██╔══╝██║         ██╔══██╗██╔════╝██╔═══██╗
 * ██║██║   ██║██║        ██║   ██║         ██████╔╝█████╗  ██║   ██║
 * ██║██║   ██║██║        ██║   ██║         ██╔══██╗██╔══╝  ██║▄▄ ██║
 * ██║╚██████╔╝╚██████╗   ██║   ███████╗    ██║  ██║███████╗╚██████╔╝
 * ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝    ╚═╝  ╚═╝╚══════╝ ╚══▀▀═╝ 
 *                          
 ***********************************************************************************/

// req {{{
/**
 * Request new RAB slices.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @arg:        ioctl() argument
 */
long pulp_rab_req(void *rab_config, unsigned long arg)
{
  int err = 0, i, j;
  long retval = 0;

  // to read from user space
  unsigned request[3];
  unsigned long n_bytes_read, n_bytes_left;
  unsigned byte;

  // what we get from user space
  unsigned size_b;

  // what get_user_pages needs
  struct page ** pages;
  unsigned       len; 
  
  // what mem_map_sg needs needs
  unsigned *      addr_start_vec;
  unsigned *      addr_end_vec;
  unsigned long * addr_offset_vec;
  unsigned *      page_idxs_start;
  unsigned *      page_idxs_end;
  unsigned        n_segments;

  // needed for cache flushing
  unsigned offset_start, offset_end;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;

  // needed for L2 TLB
  unsigned char rab_lvl;
  unsigned char rab_use_l1;
  unsigned * pfn_v_vec;
  unsigned * pfn_p_vec;
  L2Entry l2_entry_request;
  L2Entry *l2_entry = &l2_entry_request;
  unsigned n_free_slices;

  // get slice data from user space - arg already checked above
  byte = 0;
  n_bytes_left = 3*sizeof(unsigned); 
  n_bytes_read = n_bytes_left;
  while (n_bytes_read > 0) {
    n_bytes_left = __copy_from_user((void *)((char *)request+byte),
                                    (void __user *)((char *)arg+byte), n_bytes_read);
    byte += (n_bytes_read - n_bytes_left);
    n_bytes_read = n_bytes_left;
  }
     
  // parse request
  RAB_GET_FLAGS_HW(rab_slice_req->flags_hw, request[0]);    
  RAB_GET_PORT(rab_slice_req->rab_port, request[0]);
  RAB_GET_LVL(rab_lvl, request[0]);
  RAB_GET_OFFLOAD_ID(rab_slice_req->rab_mapping, request[0]);
  RAB_GET_DATE_EXP(rab_slice_req->date_exp, request[0]);
  RAB_GET_DATE_CUR(rab_slice_req->date_cur, request[0]);

  rab_slice_req->addr_start = request[1];
  size_b = request[2];
  
  rab_slice_req->addr_end = rab_slice_req->addr_start + size_b;
  n_segments = 1;

  if (DEBUG_LEVEL_RAB > 1) {
    printk(KERN_INFO "PULP: New RAB request:\n");
    printk(KERN_INFO "PULP: addr_start = %#x.\n",rab_slice_req->addr_start);
    printk(KERN_INFO "PULP: addr_end   = %#x.\n",rab_slice_req->addr_end);
    printk(KERN_INFO "PULP: flags_hw   = %#x.\n",rab_slice_req->flags_hw);
    printk(KERN_INFO "PULP: rab_port   = %d.\n",rab_slice_req->rab_port);
    printk(KERN_INFO "PULP: rab_lvl    = %d.\n",rab_lvl);
    printk(KERN_INFO "PULP: date_exp   = %d.\n",rab_slice_req->date_exp);
    printk(KERN_INFO "PULP: date_cur   = %d.\n",rab_slice_req->date_cur);
  }
  
  // check type of remapping
  if ( (rab_slice_req->rab_port == 0) || (rab_slice_req->addr_start == L3_MEM_BASE_ADDR) ) 
    rab_slice_req->flags_drv = 0x1 | 0x4;
  else  // for now, set up every request on every mapping
    rab_slice_req->flags_drv = 0x4;
  
  rab_use_l1 = 0;
  if (rab_slice_req->flags_drv & 0x1) { // constant remapping
    switch(rab_slice_req->addr_start) {
    
    case MBOX_H_BASE_ADDR:
      rab_slice_req->addr_offset = MBOX_BASE_ADDR - MBOX_SIZE_B; // Interface 0
      break;
  
    case L2_MEM_H_BASE_ADDR:
      rab_slice_req->addr_offset = L2_MEM_BASE_ADDR;
      break;

    case PULP_H_BASE_ADDR:
      rab_slice_req->addr_offset = PULP_BASE_REMOTE_ADDR;
      break;

    default: // L3_MEM_BASE_ADDR - port 1
      rab_slice_req->addr_offset = L3_MEM_H_BASE_ADDR;
    }

    len = 1;
    rab_use_l1 = 1;
  }
  else { // address translation required    
    // number of pages
    len = pulp_mem_get_num_pages(rab_slice_req->addr_start,size_b);

    // get and lock user-space pages
    err = pulp_mem_get_user_pages(&pages, rab_slice_req->addr_start, len, rab_slice_req->flags_hw & 0x4);
    if (err) {
      printk(KERN_WARNING "PULP - RAB: Locking of user-space pages failed.\n");
      return (long)err;
    }
    
    // where to place the mapping ?
    if (rab_lvl == 0) {
      // Check the number of segments and the number of available slices.
      n_segments = pulp_mem_check_num_sg(&pages, len);
      n_free_slices = pulp_rab_num_free_slices(rab_slice_req);
      if (n_free_slices < n_segments)
        rab_use_l1 = 0; // use L2 if number of free slices in L1 is not sufficient.
      else
        rab_use_l1 = 1;
    } else {
      rab_use_l1 = rab_lvl % 2;
    }        
  }

  if (rab_use_l1 == 1) { // use L1 TLB

    if ( !(rab_slice_req->flags_drv & 0x1) ) { // not constant remapping
      // virtual to physcial address translation + segmentation
      n_segments = pulp_mem_map_sg(&addr_start_vec, &addr_end_vec, &addr_offset_vec,
                                   &page_idxs_start, &page_idxs_end, &pages, len, 
                                   rab_slice_req->addr_start, rab_slice_req->addr_end);
      if ( n_segments < 1 ) {
        printk(KERN_WARNING "PULP - RAB: Virtual to physical address translation failed.\n");
        return (long)n_segments;
      }
    }

    /*
     *  setup the slices
     */ 
    // to do: protect with semaphore!?
    for (i=0; i<n_segments; i++) {
      
      if ( !(rab_slice_req->flags_drv & 0x1) ) {
        rab_slice_req->addr_start     = addr_start_vec[i];
        rab_slice_req->addr_end       = addr_end_vec[i];
        rab_slice_req->addr_offset    = addr_offset_vec[i];
        rab_slice_req->page_idx_start = page_idxs_start[i];
        rab_slice_req->page_idx_end   = page_idxs_end[i];
      }
      
      // some requests need to be set up for every mapping
      for (j=0; j<RAB_L1_N_MAPPINGS_PORT_1; j++) {
        
        if ( (rab_slice_req->rab_port == 1) && (rab_slice_req->flags_drv & 0x4) )
          rab_slice_req->rab_mapping = j;
        
        if ( rab_slice_req->rab_mapping == j ) {
          
          //  check for free field in page_ptrs list
          if ( !i && (rab_slice_req->rab_port == 1) ) {
            err = pulp_rab_page_ptrs_get_field(rab_slice_req);
            if (err) {
              return (long)err;
            }
          }
          
          // get a free slice
          err = pulp_rab_slice_get(rab_slice_req);
          if (err) {
            return (long)err;
          }
          
          // free memory of slices to be re-configured
          pulp_rab_slice_free(rab_config, rab_slice_req);
          
          // setup slice
          err = pulp_rab_slice_setup(rab_config, rab_slice_req, pages);
          if (err) {
            return (long)err;
          }
        }
      }

      // flush caches
      if ( !(rab_slice_req->flags_drv & 0x1) && !(rab_slice_req->flags_hw & 0x8) ) {
        for (j=page_idxs_start[i]; j<(page_idxs_end[i]+1); j++) {
          // flush the whole page?
          if (!i) 
            offset_start = BF_GET(addr_start_vec[i],0,PAGE_SHIFT);
          else
            offset_start = 0;
          
          if (i == (n_segments-1) )
            offset_end = BF_GET(addr_end_vec[i],0,PAGE_SHIFT);
          else 
            offset_end = PAGE_SIZE;
          
          pulp_mem_cache_flush(pages[j],offset_start,offset_end);
        }
      }
    } // for n_segments
  } 
  else { // use L2 TLB
    // virtual to physical page frame number translation for each page
    err = pulp_mem_l2_get_entries(&pfn_v_vec, &pfn_p_vec, &pages, len, rab_slice_req->addr_start);
    l2_entry->flags = rab_slice_req->flags_hw;
    
    for (i=0; i<len; i++) {
      l2_entry->pfn_v    = pfn_v_vec[i];
      l2_entry->pfn_p    = pfn_p_vec[i];
      if (pages)
        l2_entry->page_ptr = pages[i];
      else
        l2_entry->page_ptr = 0;
      
      // setup entry
      err = pulp_rab_l2_setup_entry(rab_config, l2_entry, rab_slice_req->rab_port, 0);
      if (err) {
        return (long)err;
      }
      
      // flush caches
      if ( !(rab_slice_req->flags_drv & 0x1) && !(rab_slice_req->flags_hw & 0x8) ) {
        // flush the whole page?
        pulp_mem_cache_flush(pages[i],0,PAGE_SIZE);
      }
    } // for (i=0; i<len; i++) {
  
    if ( !(rab_slice_req->flags_drv & 0x1) ) {
      kfree(pages); // No need of pages since we have the individual page ptr for L2 entry in page_ptr.
    }
  }

  // kfree
  if ( (!(rab_slice_req->flags_drv & 0x1)) && rab_use_l1) {
    kfree(addr_start_vec);
    kfree(addr_end_vec);
    kfree(addr_offset_vec);
    kfree(page_idxs_start);
    kfree(page_idxs_end);
  }    
  if (rab_use_l1 == 0) {
    kfree(pfn_v_vec);
    kfree(pfn_p_vec);
  }

  return retval;
}
// }}}

// req_striped {{{
/**
 * Request striped RAB slices.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @arg:        ioctl() argument
 */
long pulp_rab_req_striped(void *rab_config, unsigned long arg)
{
  int err = 0, i, j, k;
  long retval = 0;
  
  // to read from user space
  unsigned long n_bytes_read, n_bytes_left;
  unsigned byte;

  // what we get from user space
  unsigned size_b;
   
  // what get_user_pages needs
  struct page ** pages;
  unsigned       len; 
  
  // what mem_map_sg needs needs
  unsigned *      addr_start_vec;
  unsigned *      addr_end_vec;
  unsigned long * addr_offset_vec;
  unsigned *      page_idxs_start;
  unsigned *      page_idxs_end;
  unsigned        n_segments;

  // needed for cache flushing
  unsigned offset_start, offset_end;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;
  unsigned n_slices;

  // needed for RAB striping
  RabStripeReqUser    rab_stripe_req_user;
  RabStripeElemUser * rab_stripe_elem_user;
  RabStripeElem *     rab_stripe_elem;
  unsigned *          stripe_addr_start;
  unsigned *          stripe_addr_end;
  unsigned            addr_start;
  unsigned            addr_end;
  unsigned long       addr_offset;
  unsigned            seg_idx_start;
  unsigned            seg_idx_end;

  #ifdef PROFILE_RAB_STR 
    // reset the ARM clock counter
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
  #endif
 
  if (DEBUG_LEVEL_RAB > 1) {
    printk(KERN_INFO "PULP - RAB: New striped RAB request.\n");
  }

  /*
   * get:
   * - ID, number of elements to stripe
   * - user-space address of array holding the RabStripeElemUser structs (1 struct per element) 
   */
  // get transfer data from user space - arg already checked above
  byte = 0;
  n_bytes_left = sizeof(RabStripeReqUser);
  n_bytes_read = n_bytes_left;
  while (n_bytes_read > 0) {
    n_bytes_left = __copy_from_user((void *)((char *)(&rab_stripe_req_user)+byte),
                                    (void __user *)((char *)arg+byte), n_bytes_read);
    byte += (n_bytes_read - n_bytes_left);
    n_bytes_read = n_bytes_left;
  }
  if (DEBUG_LEVEL_RAB_STR > 1) {
    printk(KERN_INFO "PULP - RAB: id = %d, n_elements = %d\n",rab_stripe_req_user.id, rab_stripe_req_user.n_elements);
  }
  if (DEBUG_LEVEL_RAB_STR > 2) {
    printk(KERN_INFO "PULP - RAB: rab_stripe_elem_user_addr = %#x\n",rab_stripe_req_user.rab_stripe_elem_user_addr);
  }

  // allocate memory to hold the array of RabStripeElemUser structs (1 struct per element)
  rab_stripe_elem_user = (RabStripeElemUser *)
    kmalloc((size_t)(rab_stripe_req_user.n_elements*sizeof(RabStripeElemUser)),GFP_KERNEL);
  if ( rab_stripe_elem_user == NULL ) {
    printk(KERN_WARNING "PULP - RAB: Memory allocation failed.\n");
    return -ENOMEM;
  }

  /*
   * get:
   * - array of RabStripeElemUser structs
   */
  // get data from user space
  byte = 0;
  n_bytes_left = rab_stripe_req_user.n_elements*sizeof(RabStripeElemUser); 
  n_bytes_read = n_bytes_left;
  while (n_bytes_read > 0) {
    n_bytes_left = copy_from_user((void *)((char *)rab_stripe_elem_user+byte),
                                  (void __user *)((unsigned long)rab_stripe_req_user.rab_stripe_elem_user_addr+byte),
                                  n_bytes_read);
    byte += (n_bytes_read - n_bytes_left);
    n_bytes_read = n_bytes_left;
  }

  // allocate memory to hold the array of RabStripeElem structs (1 struct per element)
  rab_stripe_elem = (RabStripeElem *)
    kmalloc((size_t)(rab_stripe_req_user.n_elements*sizeof(RabStripeElem)),GFP_KERNEL);
  if ( rab_stripe_elem == NULL ) {
    printk(KERN_WARNING "PULP - RAB: Memory allocation failed.\n");
    return -ENOMEM;
  }

  // process every data element independently
  for (i=0; i<rab_stripe_req_user.n_elements; i++) {

    /*
     * preparations
     */
    // compute the maximum number of required slices (per stripe)
    n_slices = rab_stripe_elem_user[i].max_stripe_size_b/PAGE_SIZE;
    if (n_slices*PAGE_SIZE < rab_stripe_elem_user[i].max_stripe_size_b)
      n_slices++; // remainder
    n_slices++;   // non-aligned

    if (DEBUG_LEVEL_RAB_STR > 1) {
      printk(KERN_INFO "PULP - RAB: Element %d, n_slices = %d\n", i, n_slices);
    }

    // fill RabStripeElem struct for further use
    rab_stripe_elem[i].id                  = rab_stripe_elem_user[i].id;
    rab_stripe_elem[i].type                = rab_stripe_elem_user[i].type;
    rab_stripe_elem[i].n_slices_per_stripe = n_slices;
    rab_stripe_elem[i].n_stripes           = rab_stripe_elem_user[i].n_stripes;
    rab_stripe_elem[i].flags_hw            = rab_stripe_elem_user[i].flags;

    // compute the actual number of required slices
    if (rab_stripe_elem[i].type == 4)
      rab_stripe_elem[i].n_slices = 4*n_slices; // double buffering: *2 + inout: *2
    else
      rab_stripe_elem[i].n_slices = 2*n_slices; // double buffering: *2

    // set stripe_idx = stripe to configure at first update request
    if (rab_stripe_elem[i].type == 3)
      rab_stripe_elem[i].stripe_idx = 0;
    else
      rab_stripe_elem[i].stripe_idx = 1;

    // allocate memory to hold array of slice idxs - slices will be requested later
    rab_stripe_elem[i].slice_idxs = (unsigned *)
      kmalloc((size_t)(rab_stripe_elem[i].n_slices*sizeof(unsigned)),GFP_KERNEL);
    if ( rab_stripe_elem[i].slice_idxs == NULL ) {
      printk(KERN_WARNING "PULP - RAB: Memory allocation failed.\n");
      return -ENOMEM;
    }

    /*
     * get actual striping data from user space:
     * - array of stripe start/end addresses (2 32b user-space addr per stripe)
     */
    // allocate memory to hold addr_start and addr_end array
    stripe_addr_start = (unsigned *)kmalloc((size_t)(rab_stripe_elem[i].n_stripes*sizeof(unsigned)),GFP_KERNEL);
    if ( stripe_addr_start == NULL ) {
      printk(KERN_WARNING "PULP - RAB: Memory allocation failed.\n");
      return -ENOMEM;
    }
    stripe_addr_end   = (unsigned *)kmalloc((size_t)(rab_stripe_elem[i].n_stripes*sizeof(unsigned)),GFP_KERNEL);
    if ( stripe_addr_end == NULL ) {
      printk(KERN_WARNING "PULP - RAB: Memory allocation failed.\n");
      return -ENOMEM;
    }

    // get data from user space
    byte = 0;
    n_bytes_left = rab_stripe_elem_user[i].n_stripes*sizeof(unsigned);
    n_bytes_read = n_bytes_left;
    while (n_bytes_read > 0) {
      n_bytes_left = copy_from_user((void *)((char *)stripe_addr_start+byte),
                                    (void __user *)((unsigned long)rab_stripe_elem_user[i].stripe_addr_start+byte),
                                    n_bytes_read);
      byte += (n_bytes_read - n_bytes_left);
      n_bytes_read = n_bytes_left;
    }

    byte = 0;
    n_bytes_left = rab_stripe_elem_user[i].n_stripes*sizeof(unsigned);
    n_bytes_read = n_bytes_left;
    while (n_bytes_read > 0) {
      n_bytes_left = copy_from_user((void *)((char *)stripe_addr_end+byte),
                                    (void __user *)((unsigned long)rab_stripe_elem_user[i].stripe_addr_end+byte),
                                    n_bytes_read);
      byte += (n_bytes_read - n_bytes_left);
      n_bytes_read = n_bytes_left;
    }

    addr_start = stripe_addr_start[0];
    addr_end   = stripe_addr_end[rab_stripe_elem[i].n_stripes-1];
    size_b     = addr_end - addr_start;

    if (DEBUG_LEVEL_RAB_STR > 2) {
      printk(KERN_INFO "PULP - RAB: addr_start = %#x \n",addr_start);
      printk(KERN_INFO "PULP - RAB: addr_end   = %#x \n",addr_end);
      printk(KERN_INFO "PULP - RAB: size_b     = %#x \n",size_b);
    }

    /*
     * get user-space pages ready
     */
    // number of pages
    len = pulp_mem_get_num_pages(addr_start,size_b);

    #ifdef PROFILE_RAB_STR
      n_pages_setup += len;
    #endif   
    
    #ifdef PROFILE_RAB_STR
      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
    #endif   

    // get and lock user-space pages
    err = pulp_mem_get_user_pages(&pages, addr_start, len, rab_stripe_elem[i].flags_hw & 0x4);
    if (err) {
      printk(KERN_WARNING "PULP: Locking of user-space pages failed.\n");
      return err;
    }

    #ifdef PROFILE_RAB_STR
      arm_clk_cntr_value_start = arm_clk_cntr_value;
    
      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      n_cyc_get_user_pages = arm_clk_cntr_value - arm_clk_cntr_value_start;
      n_cyc_tot_get_user_pages += n_cyc_get_user_pages;
    #endif   
 
    // virtual to physcial address translation + segmentation
    n_segments = pulp_mem_map_sg(&addr_start_vec, &addr_end_vec, &addr_offset_vec,
                                 &page_idxs_start, &page_idxs_end, &pages, len, 
                                 addr_start, addr_end);
    if ( n_segments < 1 ) {
      printk(KERN_WARNING "PULP - RAB: Virtual to physical address translation failed.\n");
      return n_segments;
    }

    #ifdef PROFILE_RAB_STR
      arm_clk_cntr_value_start = arm_clk_cntr_value;
    
      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      n_cyc_map_sg = arm_clk_cntr_value - arm_clk_cntr_value_start;
      n_cyc_tot_map_sg += n_cyc_map_sg;
    #endif   

    if( !(rab_stripe_elem[i].flags_hw & 0x8) ) {
      // flush caches for all segments
      for (j=0; j<n_segments; j++) { 
        for (k=page_idxs_start[j]; k<(page_idxs_end[j]+1); k++) {
          // flush the whole page?
          if (!j) 
            offset_start = BF_GET(addr_start_vec[j],0,PAGE_SHIFT);
          else
            offset_start = 0;

          if (j == (n_segments-1) )
            offset_end = BF_GET(addr_end_vec[j],0,PAGE_SHIFT);
          else 
            offset_end = PAGE_SIZE;
  
          pulp_mem_cache_flush(pages[k],offset_start,offset_end);
        }
      }
    }

    #ifdef PROFILE_RAB_STR
      arm_clk_cntr_value_start = arm_clk_cntr_value;
    
      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      n_cyc_cache_flush = arm_clk_cntr_value - arm_clk_cntr_value_start;
      n_cyc_tot_cache_flush += n_cyc_cache_flush;
    #endif   

    /*
     * generate the striping table
     */
    // allocate memory to hold array of RabStripe structs
    rab_stripe_elem[i].stripes = (RabStripe *)
      kmalloc((size_t)rab_stripe_elem[i].n_stripes*sizeof(RabStripe),GFP_KERNEL);
    if ( rab_stripe_elem[i].stripes == NULL ) {
      printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
      return -ENOMEM;
    }

    seg_idx_start = 0;
    seg_idx_end = 0;

    // loop over stripes
    for (j=0; j<rab_stripe_elem[i].n_stripes; j++) {

      // determine the segment indices
      while ( ((seg_idx_start+1) < n_segments) && 
              (stripe_addr_start[j] >= addr_start_vec[seg_idx_start+1]) ) {
        seg_idx_start++;
      }
      while ( (seg_idx_end < n_segments) && 
              (stripe_addr_end[j] > addr_end_vec[seg_idx_end]) ) {
        seg_idx_end++;
      }

      if (DEBUG_LEVEL_RAB_STR > 3) {
        printk(KERN_INFO "PULP - RAB: Stripe %d: seg_idx_start = %d, seg_idx_end = %d \n",
               j,seg_idx_start,seg_idx_end);
      }
  
      n_slices = seg_idx_end - seg_idx_start + 1; // number of required slices
      rab_stripe_elem[i].stripes[j].n_slices = n_slices;
      if ( n_slices > (rab_stripe_elem[i].n_slices_per_stripe) ) {
        printk(KERN_WARNING "PULP - RAB: Stripe %d of Element %d touches too many memory segments.\n",j,i);
        //printk(KERN_INFO "%d slices reserved, %d segments\n",elem_cur->n_slices_per_stripe, n_segments);
        //printk(KERN_INFO "start segment = %d, end segment = %d\n",seg_idx_start,seg_idx_end);
      }

      // allocate memory to hold array of RabStripeSlice structs
      rab_stripe_elem[i].stripes[j].slice_configs = (RabStripeSlice *)
        kmalloc((size_t)n_slices*sizeof(RabStripeSlice),GFP_KERNEL);
      if ( rab_stripe_elem[i].stripes[j].slice_configs == NULL ) {
        printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
        return -ENOMEM;
      }

      // extract the addr of segments, loop over slices 
      for (k=0; k<n_slices; k++) {
        // virtual start + physical addr 
        if (k == 0) {
          addr_start  = stripe_addr_start[j];
          addr_offset = addr_offset_vec[seg_idx_start];
          // inter-page offset: for multi-page segments
          addr_offset += (unsigned long)((BF_GET(addr_start,PAGE_SHIFT,32-PAGE_SHIFT) << PAGE_SHIFT)
            - (BF_GET(addr_start_vec[seg_idx_start],PAGE_SHIFT,32-PAGE_SHIFT) << PAGE_SHIFT)); 
          // intra-page offset: no additional offset for first slice of first stripe
          if ( j != 0 )
            addr_offset += (unsigned long)(stripe_addr_start[j] & BIT_MASK_GEN(PAGE_SHIFT));
        }
        else { // ( k < n_slices )
          addr_start  = addr_start_vec [seg_idx_start + k];
          addr_offset = addr_offset_vec[seg_idx_start + k];
        }
        // virtual end addr
        if ( k < (n_slices-1) ) // end addr defined by segment end
          addr_end = addr_end_vec[seg_idx_start + k];
        else // last slice, end at stripe end
          addr_end = stripe_addr_end[j];

        // write data to table
        rab_stripe_elem[i].stripes[j].slice_configs[k].addr_start  = addr_start;
        rab_stripe_elem[i].stripes[j].slice_configs[k].addr_end    = addr_end;
        rab_stripe_elem[i].stripes[j].slice_configs[k].addr_offset = addr_offset;
      }
    }

    if (DEBUG_LEVEL_RAB_STR > 3) {
      for (j=0; j<rab_stripe_elem[i].n_stripes; j++) {
        printk(KERN_INFO "PULP - RAB: Stripe %d:\n",j);
        for (k=0; k<rab_stripe_elem[i].stripes[j].n_slices; k++) {
          printk(KERN_INFO "addr_start [%d] %#x\n",  k, rab_stripe_elem[i].stripes[j].slice_configs[k].addr_start );
          printk(KERN_INFO "addr_end   [%d] %#x\n",  k, rab_stripe_elem[i].stripes[j].slice_configs[k].addr_end   );
          printk(KERN_INFO "addr_offset[%d] %#lx\n", k, rab_stripe_elem[i].stripes[j].slice_configs[k].addr_offset);
        } 
        printk(KERN_INFO "\n");
      }
    } 

    /*
     *  request the slices
     */
    rab_slice_req->rab_port    = 1;
    rab_slice_req->rab_mapping = rab_stripe_req_user.id % RAB_L1_N_MAPPINGS_PORT_1;
    rab_slice_req->flags_hw    = rab_stripe_elem[i].flags_hw;

    // do not overwrite any remappings, the remappings will be freed manually
    rab_slice_req->date_cur = 0x1;
    rab_slice_req->date_exp = RAB_MAX_DATE; // also avoid check in pulp_rab_slice_setup

    // assign all pages to all slices
    rab_slice_req->page_idx_start = page_idxs_start[0];
    rab_slice_req->page_idx_end   = page_idxs_end[n_segments-1];

    //  check for free field in page_ptrs list
    err = pulp_rab_page_ptrs_get_field(rab_slice_req);
    if (err) {
      return err;
    }
    rab_stripe_elem[i].page_ptr_idx = rab_slice_req->page_ptr_idx;

    for (j=0; j<rab_stripe_elem[i].n_slices; j++) {
      // set up only the used slices of the first stripe, the others need just to be requested  
      if ( j > (rab_stripe_elem[i].stripes[0].n_slices-1) ) {
        rab_slice_req->addr_start  = 0;
        rab_slice_req->addr_end    = 0;
        rab_slice_req->addr_offset = 0;
        rab_slice_req->flags_hw    = rab_stripe_elem[i].flags_hw & 0xE; // do not yet enable
      }
      else {
        rab_slice_req->addr_start  = rab_stripe_elem[i].stripes[0].slice_configs[j].addr_start;
        rab_slice_req->addr_end    = rab_stripe_elem[i].stripes[0].slice_configs[j].addr_end;
        rab_slice_req->addr_offset = rab_stripe_elem[i].stripes[0].slice_configs[j].addr_offset;
        rab_slice_req->flags_hw    = rab_stripe_elem[i].flags_hw;
      }
      // force check in pulp_rab_slice_setup
      if (j == 0)
        rab_slice_req->flags_drv = 0x0; 
      else
        rab_slice_req->flags_drv = 0x2; // striped mode, not constant
  
      // get a free slice
      err = pulp_rab_slice_get(rab_slice_req);
      if (err) {
        return err;
      }
      rab_stripe_elem[i].slice_idxs[j] = rab_slice_req->rab_slice;

      // free memory of slices to be re-configured
      pulp_rab_slice_free(rab_config, rab_slice_req);
          
      // set up slice
      err = pulp_rab_slice_setup(rab_config, rab_slice_req, pages);
      if (err) {
        return err;
      }
    }

    if (DEBUG_LEVEL_RAB > 2) {
      printk(KERN_INFO "PULP - RAB: Element %d: \n",i);
      printk(KERN_INFO "PULP - RAB: Striped slices: \n");
      for (j=0; j<rab_stripe_elem[i].n_slices; j++) {
        printk(KERN_INFO "%d, ",rab_stripe_elem[i].slice_idxs[j]);
      }
      printk(KERN_INFO "\n");
    } 

    // free memory
    kfree(stripe_addr_start);
    kfree(stripe_addr_end);

    // free memory - allocated inside pulp_mem_map_sg()
    kfree(addr_start_vec);
    kfree(addr_end_vec);
    kfree(addr_offset_vec);
    kfree(page_idxs_start);
    kfree(page_idxs_end);
  } // for n_elements

  // fill RabStripeReq struct for further use
  rab_stripe_req[rab_slice_req->rab_mapping].id         = rab_stripe_req_user.id;
  rab_stripe_req[rab_slice_req->rab_mapping].n_elements = rab_stripe_req_user.n_elements;
  rab_stripe_req[rab_slice_req->rab_mapping].elements   = &rab_stripe_elem[0];
  
  // free memory
  kfree(rab_stripe_elem_user);

  #ifdef PROFILE_RAB_STR
    // read the ARM clock counter
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
    n_cyc_setup = arm_clk_cntr_value;
    n_cyc_tot_setup += n_cyc_setup;

    // write the counter values to the buffers
    n_cyc_buf_setup[idx_buf_setup] = n_cyc_setup;
    idx_buf_setup++;
    n_cyc_buf_cache_flush[idx_buf_cache_flush] = n_cyc_cache_flush;
    idx_buf_cache_flush++;
    n_cyc_buf_get_user_pages[idx_buf_get_user_pages] = n_cyc_get_user_pages;
    idx_buf_get_user_pages++;
    n_cyc_buf_map_sg[idx_buf_map_sg] = n_cyc_map_sg;
    idx_buf_map_sg++;

    // check for buffer overflow
    if ( idx_buf_setup > PROFILE_RAB_N_REQUESTS ) {
      idx_buf_setup = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_setup overflow!\n");
    }
    if ( idx_buf_cache_flush > PROFILE_RAB_N_ELEMENTS ) {
      idx_buf_cache_flush = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_cache_flush overflow!\n");
    }
    if ( idx_buf_get_user_pages > PROFILE_RAB_N_ELEMENTS ) {
      idx_buf_get_user_pages = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_get_user_pages overflow!\n");
    }
    if ( idx_buf_map_sg > PROFILE_RAB_N_ELEMENTS ) {
      idx_buf_map_sg = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_map_sg overflow!\n");
    }
 #endif

  return retval;
}
// }}}

// free {{{
/***********************************************************************************
 *
 * ██╗ ██████╗  ██████╗████████╗██╗         ███████╗██████╗ ███████╗███████╗
 * ██║██╔═══██╗██╔════╝╚══██╔══╝██║         ██╔════╝██╔══██╗██╔════╝██╔════╝
 * ██║██║   ██║██║        ██║   ██║         █████╗  ██████╔╝█████╗  █████╗  
 * ██║██║   ██║██║        ██║   ██║         ██╔══╝  ██╔══██╗██╔══╝  ██╔══╝  
 * ██║╚██████╔╝╚██████╗   ██║   ███████╗    ██║     ██║  ██║███████╗███████╗
 * ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝    ╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝
 * 
 ***********************************************************************************/

/**
 * Free RAB slices based on time code.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @arg:        ioctl() argument
 */
void pulp_rab_free(void *rab_config, unsigned long arg)
{
  int i, j, k;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;
  unsigned n_slices;
  unsigned n_mappings;

  // get current date    
  rab_slice_req->date_cur = BF_GET(arg,0,RAB_CONFIG_N_BITS_DATE);

  // check every slice on every port for every mapping
  for (i=0; i<RAB_N_PORTS; i++) {
  rab_slice_req->rab_port = i;
  
    if (i == 0) {
      n_mappings = 1;
      n_slices   = RAB_L1_N_SLICES_PORT_0;
    }
    else { // Port 1
      n_mappings = RAB_L1_N_MAPPINGS_PORT_1;
      n_slices   = RAB_L1_N_SLICES_PORT_1;
    }
  
    for (j=0; j<n_mappings; j++) {
      rab_slice_req->rab_mapping = j;
      for (k=0; k<n_slices; k++) {
        rab_slice_req->rab_slice = k;

        if (rab_slice_req->rab_port == 1) {
          rab_slice_req->flags_drv = l1.port_1.mappings[j].slices[k].flags_drv;
          if ( (rab_slice_req->flags_drv & 0x4) && (j == n_mappings-1) )
            BIT_CLEAR(rab_slice_req->flags_drv,0x4); // unlock and release pages when freeing the last mapping
        }
        else
          rab_slice_req->flags_drv = 0x1; // Port 0 can only hold constant mappings
          
        if ( !rab_slice_req->date_cur ||
             (rab_slice_req->date_cur == RAB_MAX_DATE) ||
             (pulp_rab_slice_check(rab_slice_req)) ) // free slice
          pulp_rab_slice_free(rab_config, rab_slice_req);
      }
    }
  }

  // for debugging
  //pulp_rab_mapping_print(rab_config,0xAAAA);

  // free L2TLB    
  pulp_rab_l2_invalidate_all_entries(rab_config, 1);

  return;
}
// }}}

// free_striped {{{
/**
 * Free striped RAB slices.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @arg:        ioctl() argument
 */
void pulp_rab_free_striped(void *rab_config, unsigned long arg)
{
  int i, j;
  unsigned char id;
  
  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;

  // needed for RAB striping
  RabStripeElem * rab_stripe_elem;

  #ifdef PROFILE_RAB_STR 
    // reset the ARM clock counter
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
  #endif

  // get id/rab_mapping
  id = (unsigned char)arg;
  rab_slice_req->rab_port = 1;
  rab_slice_req->rab_mapping = id % RAB_L1_N_MAPPINGS_PORT_1;

  if (DEBUG_LEVEL_RAB_STR > 1) {
    printk(KERN_INFO "PULP - RAB: id = %d, n_elements = %d\n",id, rab_stripe_req[rab_slice_req->rab_mapping].n_elements);
  }

  if ( rab_stripe_req[rab_slice_req->rab_mapping].n_elements > 0 ) {

    rab_stripe_elem = rab_stripe_req[rab_slice_req->rab_mapping].elements;

    // process every data element independently
    for (i=0; i<rab_stripe_req[rab_slice_req->rab_mapping].n_elements; i++) {

      if (DEBUG_LEVEL_RAB_STR > 1) {
        printk(KERN_INFO "PULP - RAB: Element %d:\n",i);
      }

      rab_slice_req->flags_hw = rab_stripe_elem[i].flags_hw; 

      // free the slices
      for (j=0; j<rab_stripe_elem[i].n_slices; j++){
        rab_slice_req->rab_slice = rab_stripe_elem[i].slice_idxs[j];

        // unlock and release pages when freeing the last slice
        if (j < (rab_stripe_elem[i].n_slices-1) )
          rab_slice_req->flags_drv = 0x2;
        else
          rab_slice_req->flags_drv = 0x0;

        pulp_rab_slice_free(rab_config, rab_slice_req);
      }

      // free memory
      kfree(rab_stripe_elem[i].slice_idxs);
      for (j=0; j<rab_stripe_elem[i].n_stripes; j++) {
        kfree(rab_stripe_elem[i].stripes[j].slice_configs);
      }
      kfree(rab_stripe_elem[i].stripes);
    }

    // free memory
    kfree(rab_stripe_req[rab_slice_req->rab_mapping].elements);
  
    // mark the stripe request as freed
    rab_stripe_req[rab_slice_req->rab_mapping].n_elements = 0;
  }

  #ifdef PROFILE_RAB_STR 
    // read the ARM clock counter
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
    n_cyc_cleanup = arm_clk_cntr_value;
    n_cyc_tot_cleanup += n_cyc_cleanup;
  
    n_cleanups++;
  
    // write the counter values to the buffers
    n_cyc_buf_cleanup[idx_buf_cleanup] = n_cyc_cleanup;
    idx_buf_cleanup++;

    // check for buffer overflow
    if ( idx_buf_cleanup > PROFILE_RAB_N_REQUESTS ) {
      idx_buf_cleanup = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_cleanup overflow!\n");
    }
  #endif
       
    return;
  }
// }}}

// soc_mh_ena {{{
/**
 * TODO: documentation
 */
int pulp_rab_soc_mh_ena(struct task_struct* task, void* rab_config, void* mbox)
{
  // Get physical address of page global directory (i.e., the process-specific top-level page
  // table).
  const pgd_t* pgd_ptr = current->mm->pgd;
  BUG_ON(pgd_none(*pgd_ptr));
  const unsigned long pgd_pa = (((unsigned long)(pgd_val(*pgd_ptr)) & PHYS_MASK) >> 2) << 2;
  printk(KERN_DEBUG "PULP RAB SoC MH PGD PA: 0x%010lx\n", pgd_pa);

  // Set up a RAB mapping between physical address space of page tables and virtual address
  // space of PULP core running the PTW.
  RabSliceReq rab_slice_req;
  rab_slice_req.date_cur        = 0;
  rab_slice_req.date_exp        = RAB_MAX_DATE_MH;
  rab_slice_req.page_ptr_idx    = RAB_L1_N_SLICES_PORT_1-1;
  rab_slice_req.page_idx_start  = 0;
  rab_slice_req.page_idx_end    = 0;
  rab_slice_req.rab_port        = 1;
  rab_slice_req.rab_mapping     = 0;
  rab_slice_req.rab_slice       = 4; // TODO: generalize
  rab_slice_req.flags_drv       = 0b001;    // not setup in every mapping, not striped, constant
  rab_slice_req.addr_start      = PGD_BASE_ADDR;
  rab_slice_req.addr_end        = rab_slice_req.addr_start + ((PTRS_PER_PGD-1) << 3);
  rab_slice_req.addr_offset     = pgd_pa;
  rab_slice_req.flags_hw        = 0b0011;  // disable ACP, disable write, enable read, enable slice

  printk(KERN_DEBUG "PULP RAB SoC MH slice %u request: start 0x%08x, end 0x%08x, off 0x%010lx\n",
        rab_slice_req.rab_slice, rab_slice_req.addr_start, rab_slice_req.addr_end,
        rab_slice_req.addr_offset);

  // Request to setup RAB slice.  If this fails, the SoC MH cannot be enabled because the PTW would
  // access the wrong physical address.
  const int retval = pulp_rab_slice_setup(rab_config, &rab_slice_req, NULL);
  if (retval != 0) {
    printk(KERN_ERR "PULP RAB SoC MH slice %u request failed!\n", rab_slice_req.rab_slice);
    return retval;
  }

  // Write PGD to mailbox.  TODO: This actually is unnecessary because a constant RAB slice to the
  // physical address exists anyway.  However, we need some way to tell a *single core* that it
  // should handle RAB misses.
  iowrite32(pgd_pa, (void *)((unsigned long)mbox+MBOX_WRDATA_OFFSET_B));

  return 0;
}
// }}}

// }}}

// Mailbox Requests {{{
/***********************************************************************************
 *
 * ███╗   ███╗██████╗  ██████╗ ██╗  ██╗    ██████╗ ███████╗ ██████╗ 
 * ████╗ ████║██╔══██╗██╔═══██╗╚██╗██╔╝    ██╔══██╗██╔════╝██╔═══██╗
 * ██╔████╔██║██████╔╝██║   ██║ ╚███╔╝     ██████╔╝█████╗  ██║   ██║
 * ██║╚██╔╝██║██╔══██╗██║   ██║ ██╔██╗     ██╔══██╗██╔══╝  ██║▄▄ ██║
 * ██║ ╚═╝ ██║██████╔╝╚██████╔╝██╔╝ ██╗    ██║  ██║███████╗╚██████╔╝
 * ╚═╝     ╚═╝╚═════╝  ╚═════╝ ╚═╝  ╚═╝    ╚═╝  ╚═╝╚══════╝ ╚══▀▀═╝ 
 *                          
 ***********************************************************************************/

// update {{{
/**
 * Update the RAB configuration of slices configured in a striped mode.
 * -> Actually, this is the bottom handler and it should go into a tasklet.
 *
 * @update_req: update request identifier, specifies which elements to
 *              update in which way (advance to next stripe, wrap)
 */
void pulp_rab_update(unsigned update_req)
{
  int i, j;
  unsigned elem_mask, type;
  unsigned rab_mapping;
  unsigned stripe_idx, idx_mask, n_slices, offset;
  RabStripeElem * elem;

  elem_mask = 0;
  RAB_UPDATE_GET_ELEM(elem_mask, update_req);
  type = 0;
  RAB_UPDATE_GET_TYPE(type, update_req); 

  if (DEBUG_LEVEL_RAB_STR > 0) {
    printk(KERN_INFO "PULP - RAB: update requested, elem_mask = %#x, type = %d\n", elem_mask, type);
  }
  
  #ifdef PROFILE_RAB_STR 
    // stop the PULP timer  
    iowrite32(0x1,(void *)((unsigned long)(pulp->clusters)+TIMER_STOP_OFFSET_B));
  
    // read the PULP timer
    n_cyc_response = ioread32((void *)((unsigned long)(pulp->clusters)
      +TIMER_GET_TIME_LO_OFFSET_B));
    n_cyc_tot_response += n_cyc_response;
  
    // reset the ARM clock counter
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
  #endif
  
  rab_mapping = (unsigned)pulp_rab_mapping_get_active();

  // process every data element independently
  for (i=0; i<rab_stripe_req[rab_mapping].n_elements; i++) {

    if ( !(elem_mask & (0x1 << i)) )
      continue;

    elem = &rab_stripe_req[rab_mapping].elements[i];
    
    n_slices   = elem->n_slices_per_stripe;
    stripe_idx = elem->stripe_idx;
    if ( elem->type == 4 ) // inout
      idx_mask = 0x3;
    else // in, out
      idx_mask = 0x1;

    // process every slice independently
    for (j=0; j<n_slices; j++) {
      offset = 0x20*(1*RAB_L1_N_SLICES_PORT_0+elem->slice_idxs[(stripe_idx & idx_mask)*n_slices+j]);

      if (DEBUG_LEVEL_RAB_STR > 3) {
        printk("stripe_idx = %d, n_slices = %d, j = %d, offset = %#x\n",
          stripe_idx, elem->stripes[stripe_idx].n_slices, j, offset);
      }

      // deactivate slice
      iowrite32(0x0,(void *)((unsigned long)(pulp->rab_config)+offset+0x38));

      // set up new translations rule
      if ( j < elem->stripes[stripe_idx].n_slices ) {
        iowrite32(elem->stripes[stripe_idx].slice_configs[j].addr_start,  (void *)((unsigned long)(pulp->rab_config)+offset+0x20)); // start_addr
        iowrite32(elem->stripes[stripe_idx].slice_configs[j].addr_end,    (void *)((unsigned long)(pulp->rab_config)+offset+0x28)); // end_addr
        IOWRITE_L(elem->stripes[stripe_idx].slice_configs[j].addr_offset, (void *)((unsigned long)(pulp->rab_config)+offset+0x30)); // offset  
        iowrite32(elem->flags_hw,                                         (void *)((unsigned long)(pulp->rab_config)+offset+0x38));
      #ifdef PROFILE_RAB_STR 
        n_slices_updated++;
      #endif
      }
    }
    // increase stripe idx counter
    elem->stripe_idx++;

    #if defined(PROFILE_RAB_STR) || defined(JPEG) 
      if (elem->stripe_idx == elem->n_stripes) {
        elem->stripe_idx = 0;
        if (DEBUG_LEVEL_RAB_STR > 0) {
          printk(KERN_INFO "PULP - RAB: stripe table wrap around.\n");
        }
      }
    #endif
  }

  // signal ready to PULP
  iowrite32(HOST_READY,(void *)((unsigned long)(pulp->mbox)+MBOX_WRDATA_OFFSET_B));
        
  #ifdef PROFILE_RAB_STR 
    // read the ARM clock counter
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
    n_cyc_update = arm_clk_cntr_value;
    n_cyc_tot_update += n_cyc_update;
    
    n_updates++;
    
    // write the counter values to the buffers
    n_cyc_buf_response[idx_buf_response] = n_cyc_response;
    idx_buf_response++;
    n_cyc_buf_update[idx_buf_update] = n_cyc_update;
    idx_buf_update++;
  
    // check for buffer overflow
    if ( idx_buf_response > PROFILE_RAB_N_UPDATES ) {
      idx_buf_response = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_response overflow!\n");
    }
    if ( idx_buf_update > PROFILE_RAB_N_UPDATES ) {
      idx_buf_update = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_update overflow!\n");
    }
  #endif    
  
  if (DEBUG_LEVEL_RAB_STR > 0) {
    printk(KERN_INFO "PULP - RAB: update completed.\n");
  }

  return;
}
// }}}

// switch {{{
/**
 * Switch the hardware configuration of the RAB.
 */
void pulp_rab_switch(void)
{
  int i;
  unsigned rab_mapping;

  if (DEBUG_LEVEL_RAB > 0) {
    printk(KERN_INFO "PULP - RAB: switch requested.\n");
  }
  
  // for debugging
  //pulp_rab_mapping_print(pulp->rab_config, 0xAAAA);
  
  // switch RAB mapping
  rab_mapping = (pulp_rab_mapping_get_active()) ? 0 : 1;
  pulp_rab_mapping_switch(pulp->rab_config,rab_mapping);

  // reset stripe idxs for striped mappings 
  for (i=0; i<rab_stripe_req[rab_mapping].n_elements; i++) {

    if ( rab_stripe_req[rab_mapping].elements[i].type == 3 ) // out
      rab_stripe_req[rab_mapping].elements[i].stripe_idx = 0;
    else  // in, inout
      rab_stripe_req[rab_mapping].elements[i].stripe_idx = 1;
  }

  if (DEBUG_LEVEL_RAB > 0) {
    printk(KERN_INFO "PULP - RAB: switch completed.\n");
  }
    
  // for debugging
  //pulp_rab_mapping_print(pulp->rab_config, 0xAAAA);
  
  // signal ready to PULP
  iowrite32(HOST_READY,(void *)((unsigned long)(pulp->mbox)+MBOX_WRDATA_OFFSET_B));

  return;
}
// }}}

// }}}

// Miss Handling {{{
/***********************************************************************************
 *
 * ███╗   ███╗██╗  ██╗██████╗ 
 * ████╗ ████║██║  ██║██╔══██╗
 * ██╔████╔██║███████║██████╔╝
 * ██║╚██╔╝██║██╔══██║██╔══██╗
 * ██║ ╚═╝ ██║██║  ██║██║  ██║
 * ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
 *
 ***********************************************************************************/

// mh_ena {{{
/**
 * Check whether a particular slice has expired. Returns 1 if slice
 * has expired, 0 otherwise.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @arg:        ioctl() argument
 */
long pulp_rab_mh_ena(void *rab_config, unsigned long arg)
{

  // to read from user space
  unsigned request[2];
  unsigned long n_bytes_read, n_bytes_left;
  unsigned byte;

  // get slice data from user space - arg already checked above
  byte = 0;
  n_bytes_left = 2*sizeof(unsigned); 
  n_bytes_read = n_bytes_left;
  while (n_bytes_read > 0) {
    n_bytes_left = __copy_from_user((void *)((char *)request+byte),
                           (void __user *)((char *)arg+byte), n_bytes_read);
    byte += (n_bytes_read - n_bytes_left);
    n_bytes_read = n_bytes_left;
  }
  rab_mh_acp = request[0];
  rab_mh_lvl = request[1];

  // create workqueue for RAB miss handling
  //rab_mh_wq = alloc_workqueue("%s", WQ_UNBOUND | WQ_HIGHPRI, 0, rab_mh_wq_name);
  rab_mh_wq = alloc_workqueue("%s", WQ_UNBOUND | WQ_HIGHPRI, 1, rab_mh_wq_name); // single-threaded workqueue for strict ordering
  if (rab_mh_wq == NULL) {
    printk(KERN_WARNING "PULP: Allocation of workqueue for RAB miss handling failed.\n");
    return -ENOMEM;
  }
  // initialize the workqueue
  INIT_WORK(&rab_mh_w, (void *)pulp_rab_handle_miss);
  
  // register information of the calling user-space process
  user_task = current;
  user_mm = current->mm;

  // enable - protect with semaphore?
  rab_mh = 1;
  rab_mh_date = 1;

  printk(KERN_INFO "PULP: RAB miss handling enabled.\n");

  return 0;
}
// }}}

// mh_dis {{{
/**
 * Disable the worker thread and destroy the workqueue.
 */
void pulp_rab_mh_dis(void)
{

  // disable - protect with semaphore?
  rab_mh = 0;
  rab_mh_date = 0;

  // flush and destroy the workqueue
  destroy_workqueue(rab_mh_wq);

  printk(KERN_INFO "PULP: RAB miss handling disabled.\n");

  return;
}
// }}}

// mh_sched {{{
/**
 * Append work to the worker thread running in process context.
 */
unsigned pulp_rab_mh_sched(void)
{
  if (rab_mh) { 
    #ifdef PROFILE_RAB_MH
      int i;
      // read PE timers
      for (i=0; i<N_CORES_TIMER; i++) {
        n_cyc_response_tmp[i] = ioread32((void *)((unsigned long)(pulp->clusters)
          +TIMER_GET_TIME_LO_OFFSET_B+(i+1)*PE_TIMER_OFFSET_B));
      }
    #endif
    schedule_work(&rab_mh_w);
  }

  return rab_mh;
}
// }}}

// handle_miss {{{
/**
 * Handle RAB misses. This is the actual miss handling routine executed
 * by the worker thread in process context.
 */
void pulp_rab_handle_miss(unsigned unused)
{
  int err, i, j, handled, result;
  unsigned id, id_pe, id_cluster;
  unsigned addr_start;
  unsigned long addr_phys;
    
  // what get_user_pages needs
  unsigned long start;
  struct page **pages;
  unsigned write;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;

  // needed for L2 TLB
  L2Entry l2_entry_request;
  L2Entry *l2_entry = &l2_entry_request;
  int use_l1 = 1;
  int err_l2 = 0;

  if (DEBUG_LEVEL_RAB_MH > 1)
    printk(KERN_INFO "PULP: RAB miss handling routine started.\n");
  
  // empty miss-handling FIFOs
  for (i=0; i<RAB_MH_FIFO_DEPTH; i++) {
    rab_mh_addr[i] = IOREAD_L((void *)((unsigned long)(pulp->rab_config)+RAB_MH_ADDR_FIFO_OFFSET_B));
    rab_mh_id[i]   = IOREAD_L((void *)((unsigned long)(pulp->rab_config)+RAB_MH_ID_FIFO_OFFSET_B));
    
    // detect empty FIFOs
    #if PLATFORM != JUNO    
      if ( rab_mh_id[i] & 0x80000000 )
        break;
    #else
      if ( rab_mh_id[i] & 0x8000000000000000 )
        break;
    #endif      

    if (DEBUG_LEVEL_RAB_MH > 0) {
      printk(KERN_INFO "PULP: RAB miss - i = %d, date = %#x, id = %#x, addr = %#x\n",
       i, rab_mh_date, rab_mh_id[i], rab_mh_addr[i]);
    }
     
    // handle every miss separately
    err = 0;
    addr_start = rab_mh_addr[i];
    id = rab_mh_id[i];
    
    // check the ID
    id_pe      = BF_GET(id, 0, AXI_ID_WIDTH_CORE);
    id_cluster = BF_GET(id, AXI_ID_WIDTH_CLUSTER + AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_SOC);
    // - 0x3;
    //id_cluster = BF_GET(id, AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_CLUSTER) - 0x2;

    // identify RAB port - for now, only handle misses on Port 1: PULP -> Host
    if ( BF_GET(id, AXI_ID_WIDTH, 1) )
      rab_slice_req->rab_port = 1;    
    else {
      printk(KERN_WARNING "PULP: Cannot handle RAB miss on ports different from Port 1.\n");
      printk(KERN_WARNING "PULP: RAB miss - id = %#x, addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);
      continue;
    }

    // only handle misses from PEs' data interfaces
    if ( ( BF_GET(id, AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_CLUSTER) != 0x2 ) ||
         ( (id_cluster < 0) || (id_cluster > (N_CLUSTERS-1)) ) || 
         ( (id_pe < 0) || (id_pe > N_CORES-1) ) ) {
      printk(KERN_WARNING "PULP: Can only handle RAB misses originating from PE's data interfaces. id = %#x | addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);
      // for debugging
      //      pulp_rab_mapping_print(pulp->rab_config,0xAAAA);

      continue;
    }
  
    #ifdef PROFILE_RAB_MH
      // read the PE timer
      n_cyc_schedule = ioread32((void *)((unsigned long)(pulp->clusters)
        +TIMER_GET_TIME_LO_OFFSET_B+(id_pe+1)*PE_TIMER_OFFSET_B));
      n_cyc_tot_schedule += n_cyc_schedule;
    
      // save resp_tmp
      n_cyc_response = n_cyc_response_tmp[id_pe];
      n_cyc_tot_response += n_cyc_response;
    
      n_misses++;
    #endif

    // check if there has been a miss to the same page before
    handled = 0;    
    for (j=0; j<i; j++) {
      if ( (addr_start & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned)*8-PAGE_SHIFT))
           == (rab_mh_addr[j] & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned)*8-PAGE_SHIFT)) ) {
        handled = 1;
        if (DEBUG_LEVEL_RAB_MH > 0) {
          printk(KERN_WARNING "PULP: Already handled a miss to this page.\n");
          // for debugging only - deactivate fetch enable
          // iowrite32(0xC0000000,(void *)((unsigned long)(pulp->gpio)+0x8));
        }
        break;
      }
    }

    // handle a miss
    if (!handled) {
  
      #ifdef PROFILE_RAB_MH
        // reset the ARM clock counter
        asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
      
        // read the ARM clock counter
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      
        n_first_misses++;
      #endif

      if (DEBUG_LEVEL_RAB_MH > 1) {
        printk(KERN_INFO "PULP: Trying to handle RAB miss to user-space virtual address %#x.\n",addr_start);
      }

      // what get_user_pages returns
      pages = (struct page **)kmalloc((size_t)(sizeof(struct page *)),GFP_KERNEL);
      if ( pages == NULL ) {
        printk(KERN_WARNING "PULP: Memory allocation failed.\n");
        err = 1;
        goto miss_handling_error;
      }

      // align address to page border / 4kB
      start = (unsigned long)(addr_start) & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned long)*8-PAGE_SHIFT);
      
      // get pointer to user-space buffer and lock it into memory, get a single page
      // try read/write mode first - fall back to read only
      write = 1;
      down_read(&user_task->mm->mmap_sem);
      result = get_user_pages(user_task, user_task->mm, start, 1, write, 0, pages, NULL);
      if ( result == -EFAULT ) {
        write = 0;
        result = get_user_pages(user_task, user_task->mm, start, 1, write, 0, pages, NULL);
      }
      up_read(&user_task->mm->mmap_sem);
      //current->mm = user_task->mm;
      //result = get_user_pages_fast(start, 1, 1, pages);
      if (result != 1) {
        printk(KERN_WARNING "PULP: Could not get requested user-space virtual address %#x.\n", addr_start);
        err = 1;
        goto miss_handling_error;
      }
      
      // virtual-to-physical address translation
      addr_phys = (unsigned long)page_to_phys(pages[0]);
      if (DEBUG_LEVEL_MEM > 1) {
        printk(KERN_INFO "PULP: Physical address = %#lx\n",addr_phys);
      }

      #ifdef PROFILE_RAB_MH
        arm_clk_cntr_value_start = arm_clk_cntr_value;
      
        // read the ARM clock counter
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
        n_cyc_get_user_pages = arm_clk_cntr_value - arm_clk_cntr_value_start;
        n_cyc_tot_get_user_pages += n_cyc_get_user_pages;
      #endif
      
      // fill rab_slice_req structure
      // allocate a fixed number of slices to each PE???
      // works for one port only!!!
      rab_slice_req->date_cur = (unsigned char)rab_mh_date;
      rab_slice_req->date_exp = (unsigned char)rab_mh_date; 
      rab_slice_req->page_idx_start = 0;
      rab_slice_req->page_idx_end   = 0;
      rab_slice_req->rab_mapping = (unsigned)pulp_rab_mapping_get_active();
      rab_slice_req->flags_drv = 0;

      rab_slice_req->addr_start  = (unsigned)start;
      rab_slice_req->addr_end    = (unsigned)start + PAGE_SIZE;
      rab_slice_req->addr_offset = addr_phys;
      RAB_SET_PROT(rab_slice_req->flags_hw,(write << 2) | 0x2 | 0x1);
      RAB_SET_ACP(rab_slice_req->flags_hw,rab_mh_acp);

      // Check which TLB level to use.
      // If rab_mh_lvl is 0, enter the entry in L1 if free slice is available. If not, enter in L2.
      if (rab_mh_lvl == 2) {
        use_l1 = 0;
      } else {
        // get a free slice
        use_l1 = 1;
        err = pulp_rab_slice_get(rab_slice_req);
        if (err) {
          if (rab_mh_lvl == 1) {
            printk(KERN_WARNING "PULP: RAB miss handling error 2.\n");
            goto miss_handling_error;
          }
          else
            use_l1 = 0;
        }
      }
      err = 0;
      if (use_l1 == 0) { // Use L2
        l2_entry->flags = rab_slice_req->flags_hw;
        l2_entry->pfn_v = (unsigned)(start >> PAGE_SHIFT);
        l2_entry->pfn_p = (unsigned)(addr_phys >> PAGE_SHIFT);
        l2_entry->page_ptr = pages[0];
        err_l2 = pulp_rab_l2_setup_entry(pulp->rab_config, l2_entry, rab_slice_req->rab_port, 1);   
      } else { // Use L1
        // free memory of slices to be re-configured
        pulp_rab_slice_free(pulp->rab_config, rab_slice_req);

        // check for free field in page_ptrs list
        err = pulp_rab_page_ptrs_get_field(rab_slice_req);
        if (err) {
          printk(KERN_WARNING "PULP: RAB miss handling error 0.\n");
          goto miss_handling_error;
        }
                
        // setup slice
        err = pulp_rab_slice_setup(pulp->rab_config, rab_slice_req, pages);
        if (err) {
          printk(KERN_WARNING "PULP: RAB miss handling error 1.\n");
          goto miss_handling_error;
        }        
      } // if (use_l1 == 0)

      #ifdef PROFILE_RAB_MH
        arm_clk_cntr_value_start = arm_clk_cntr_value;
      
        // read the ARM clock counter
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
        n_cyc_setup = arm_clk_cntr_value - arm_clk_cntr_value_start;
        n_cyc_tot_setup += n_cyc_setup;
      #endif

      // flush the entire page from the caches if ACP is not used
      if (!rab_mh_acp)
        pulp_mem_cache_flush(pages[0],0,PAGE_SIZE);

      if (use_l1 == 0)
        kfree(pages);

      if (rab_mh_lvl == 1)
        if (rab_slice_req->rab_slice == (RAB_L1_N_SLICES_PORT_1 - 1)) {
          rab_mh_date++;
          if (rab_mh_date > RAB_MAX_DATE_MH)
            rab_mh_date = 0;
          //printk(KERN_INFO "rab_mh_date = %d\n",rab_mh_date);
        }

      #ifdef PROFILE_RAB_MH
        arm_clk_cntr_value_start = arm_clk_cntr_value;
      
        // read the ARM clock counter
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
        n_cyc_cache_flush = arm_clk_cntr_value - arm_clk_cntr_value_start;
        n_cyc_tot_cache_flush += n_cyc_cache_flush;
      #endif
    }
    
    // // for debugging
    // pulp_rab_mapping_print(pulp->rab_config,0xAAAA);

    // wake up the sleeping PE
    iowrite32(BIT_N_SET(id_pe),
              (void *)((unsigned long)(pulp->clusters) + CLUSTER_SIZE_B*id_cluster
                       + CLUSTER_PERIPHERALS_OFFSET_B + BBMUX_CLKGATE_OFFSET_B + GP_2_OFFSET_B));

    /*
     * printk(KERN_INFO "WAKEUP: value = %#x, address = %#x\n",BIT_N_SET(id_pe), CLUSTER_SIZE_B*id_cluster \
     *        + CLUSTER_PERIPHERALS_OFFSET_B + BBMUX_CLKGATE_OFFSET_B + GP_2_OFFSET_B );
     */

#ifdef PROFILE_RAB_MH
    // read the PE timer
    n_cyc_update = ioread32((void *)((unsigned long)(pulp->clusters)
      +TIMER_GET_TIME_LO_OFFSET_B+(id_pe+1)*PE_TIMER_OFFSET_B));
    
    // write the counter values to the buffers
    n_cyc_buf_response[idx_buf_response] = n_cyc_response;
    idx_buf_response++;
    n_cyc_buf_schedule[idx_buf_schedule] = n_cyc_schedule;
    idx_buf_schedule++;
    n_cyc_buf_get_user_pages[idx_buf_get_user_pages] = n_cyc_get_user_pages;
    idx_buf_get_user_pages++;
    n_cyc_buf_setup[idx_buf_setup] = n_cyc_setup;
    idx_buf_setup++;
    n_cyc_buf_cache_flush[idx_buf_cache_flush] = n_cyc_cache_flush;
    idx_buf_cache_flush++;
    n_cyc_buf_update[idx_buf_update] = n_cyc_update;
    idx_buf_update++;

    // check for buffer overflow
    if ( idx_buf_response > PROFILE_RAB_N_UPDATES ) {
      idx_buf_response = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_response overflow!\n");
    }
    if ( idx_buf_schedule > PROFILE_RAB_N_UPDATES ) {
      idx_buf_schedule = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_schedule overflow!\n");
    }
    if ( idx_buf_get_user_pages > PROFILE_RAB_N_ELEMENTS ) {
      idx_buf_get_user_pages = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_get_user_pages overflow!\n");
    }
    if ( idx_buf_setup > PROFILE_RAB_N_REQUESTS ) {
      idx_buf_setup = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_setup overflow!\n");
    }
    if ( idx_buf_cache_flush > PROFILE_RAB_N_ELEMENTS ) {
      idx_buf_cache_flush = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_cache_flush overflow!\n");
    }
    if ( idx_buf_update > PROFILE_RAB_N_UPDATES ) {
      idx_buf_update = 0;
      printk(KERN_WARNING "PULP - RAB: n_cyc_buf_update overflow!\n");
    }

    // update accumulators in shared memory 
    iowrite32(n_cyc_tot_response,       (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_RESPONSE_OFFSET_B));
    iowrite32(n_cyc_tot_update,         (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_UPDATE_OFFSET_B));
    iowrite32(n_cyc_tot_setup,          (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_SETUP_OFFSET_B));
    iowrite32(n_cyc_tot_cache_flush,    (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_CACHE_FLUSH_OFFSET_B));
    iowrite32(n_cyc_tot_get_user_pages, (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_GET_USER_PAGES_OFFSET_B));
    iowrite32(n_cyc_tot_schedule,       (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_SCHEDULE_OFFSET_B));
    iowrite32(n_first_misses,           (void *)((unsigned long)(pulp->l3_mem)+N_FIRST_MISSES_OFFSET_B));
    iowrite32(n_misses,                 (void *)((unsigned long)(pulp->l3_mem)+N_MISSES_OFFSET_B));

#endif
    // error handling
  miss_handling_error:
    if (err) {
      printk(KERN_WARNING "PULP: Cannot handle RAB miss: id = %#x, addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);

      // for debugging
      pulp_rab_mapping_print(pulp->rab_config,0xAAAA);
    }

  }
  if (DEBUG_LEVEL_RAB_MH > 1)
    printk(KERN_INFO "PULP: RAB miss handling routine finished.\n");

  return;
}
// }}}

// }}}

// AX Logger {{{
#if PLATFORM == JUNO
  /***********************************************************************************
   *
   *  █████╗ ██╗  ██╗    ██╗      ██████╗  ██████╗ 
   * ██╔══██╗╚██╗██╔╝    ██║     ██╔═══██╗██╔════╝ 
   * ███████║ ╚███╔╝     ██║     ██║   ██║██║  ███╗
   * ██╔══██║ ██╔██╗     ██║     ██║   ██║██║   ██║
   * ██║  ██║██╔╝ ██╗    ███████╗╚██████╔╝╚██████╔╝
   * ╚═╝  ╚═╝╚═╝  ╚═╝    ╚══════╝ ╚═════╝  ╚═════╝ 
   * 
   ***********************************************************************************/                                             

  // ax_log_init {{{
  /**
   * Initialize the AX logger, i.e., allocate kernel buffer memory and clear
   * the hardware buffer. 
   */
  int pulp_rab_ax_log_init(void)
  {
    unsigned gpio, ready, status;
  
    // allocate memory for the log buffers
    rab_ar_log_buf = (unsigned *)vmalloc(RAB_AX_LOG_BUF_SIZE_B);
    rab_aw_log_buf = (unsigned *)vmalloc(RAB_AX_LOG_BUF_SIZE_B);
    if ( (rab_ar_log_buf == NULL) || (rab_aw_log_buf == NULL)) {
      printk(KERN_WARNING "PULP: Could not allocate kernel memory for RAB AX log buffer.\n");
      return -ENOMEM;
    }
  
    // initialize indices
    rab_ar_log_buf_idx = 0;
    rab_aw_log_buf_idx = 0;

    // clear AX log
    gpio = 0;
    BIT_SET(gpio,BF_MASK_GEN(GPIO_RST_N,1));
    BIT_SET(gpio,BF_MASK_GEN(GPIO_CLK_EN,1));
    BIT_SET(gpio,BF_MASK_GEN(GPIO_RAB_AR_LOG_CLR,1));
    BIT_SET(gpio,BF_MASK_GEN(GPIO_RAB_AW_LOG_CLR,1));
    iowrite32(gpio,(void *)((unsigned long)(pulp->gpio)+0x8));
  
    // wait for ready
    ready = 0;
    while ( !ready ) {
      udelay(25);
      status = ioread32((void *)((unsigned long)pulp->gpio));
      ready = BF_GET(status, GPIO_RAB_AR_LOG_RDY, 1)
        && BF_GET(status, GPIO_RAB_AW_LOG_RDY, 1);
    }
  
    // remove the AX log clear
    BIT_CLEAR(gpio,BF_MASK_GEN(GPIO_RAB_AR_LOG_CLR,1));
    BIT_CLEAR(gpio,BF_MASK_GEN(GPIO_RAB_AW_LOG_CLR,1));
    iowrite32(gpio,(void *)((unsigned long)(pulp->gpio)+0x8));
  
    return 0;
  }
  // }}}

  // ax_log_free {{{
  /**
   * Free the kernel buffer memory previously allocated using
   * pulp_rab_ax_log_init().
   */
  void pulp_rab_ax_log_free(void)
  {
    vfree(rab_ar_log_buf);
    vfree(rab_aw_log_buf);
  
    return;
  }
  // }}}

  // ax_log_read {{{
  /**
   * Read out the RAB AX Logger to the kernel buffers rab_ax_log_buf.
   *
   * @pulp_cluster_select: cluster select flag, needed to set fetch enable
   *                       correctly when manipulating the GPIO register
   * @clear:               specifies whether the logger should be cleared
   *                       (PULP is clock-gated during this procedure)
   */
  void pulp_rab_ax_log_read(unsigned pulp_cluster_select, unsigned clear)
  {
    unsigned i;
    unsigned addr, ts, meta;
  
    unsigned gpio, gpio_init;
    unsigned status, ready;
  
    gpio = 0;
    BIT_SET(gpio,BF_MASK_GEN(GPIO_RST_N,1));
    BIT_SET(gpio,BF_MASK_GEN(GPIO_CLK_EN,1));
    BIT_SET(gpio,BF_MASK_GEN(GPIO_RAB_AX_LOG_EN,1));
    BF_SET(gpio, pulp_cluster_select, 0, N_CLUSTERS);
  
    gpio_init = gpio;
  
    // disable clocks
    if (clear) {
      BIT_CLEAR(gpio,BF_MASK_GEN(GPIO_CLK_EN,1));
      iowrite32(gpio, (void *)((unsigned long)(pulp->gpio)+0x8));
    }
  
    // read out AR log
    for (i=0; i<(RAB_AX_LOG_SIZE_B/4/3); i++) {
      ts   = ioread32((void *)((unsigned long)(pulp->rab_ar_log)+(i*3+0)*4));
      meta = ioread32((void *)((unsigned long)(pulp->rab_ar_log)+(i*3+1)*4));
      addr = ioread32((void *)((unsigned long)(pulp->rab_ar_log)+(i*3+2)*4));
  
      if ( (ts == 0) && (meta == 0) && (addr == 0) )
        break;
  
      rab_ar_log_buf[rab_ar_log_buf_idx+0] = ts;
      rab_ar_log_buf[rab_ar_log_buf_idx+1] = meta;
      rab_ar_log_buf[rab_ar_log_buf_idx+2] = addr;
  
      rab_ar_log_buf_idx += 3;
  
      if ( rab_ar_log_buf_idx > (RAB_AX_LOG_BUF_SIZE_B/4) ) {
        rab_ar_log_buf_idx = 0;
        printk(KERN_WARNING "PULP - RAB: AR log buf overflow!\n");
      }
    }
  
    // clear AR log
    if (clear) {
      BIT_SET(gpio,BF_MASK_GEN(GPIO_RAB_AR_LOG_CLR,1));
      iowrite32(gpio, (void *)((unsigned long)(pulp->gpio)+0x8));
    }
  
    // read out AW log
    for (i=0; i<(RAB_AX_LOG_SIZE_B/4/3); i++) {
      ts   = ioread32((void *)((unsigned long)(pulp->rab_aw_log)+(i*3+0)*4));
      meta = ioread32((void *)((unsigned long)(pulp->rab_aw_log)+(i*3+1)*4));
      addr = ioread32((void *)((unsigned long)(pulp->rab_aw_log)+(i*3+2)*4));
  
      if ( (ts == 0) && (meta == 0) && (addr == 0) )
        break;
  
      rab_aw_log_buf[rab_aw_log_buf_idx+0] = ts;
      rab_aw_log_buf[rab_aw_log_buf_idx+1] = meta;
      rab_aw_log_buf[rab_aw_log_buf_idx+2] = addr;
      rab_aw_log_buf_idx += 3;
  
      if ( rab_aw_log_buf_idx > (RAB_AX_LOG_BUF_SIZE_B/4) ) {
        rab_aw_log_buf_idx = 0;
        printk(KERN_WARNING "PULP - RAB: AW log buf overflow!\n");
      }
    }
  
    // clear AW log
    if (clear) {
      BIT_SET(gpio,BF_MASK_GEN(GPIO_RAB_AR_LOG_CLR,1));
      BIT_SET(gpio,BF_MASK_GEN(GPIO_RAB_AW_LOG_CLR,1));
      iowrite32(gpio, (void *)((unsigned long)(pulp->gpio)+0x8));
  
      // wait for ready
      ready = 0;
      while ( !ready ) {
        udelay(25);
        status = ioread32((void *)((unsigned long)pulp->gpio));
        ready = BF_GET(status, GPIO_RAB_AR_LOG_RDY, 1)
          && BF_GET(status, GPIO_RAB_AW_LOG_RDY, 1);
      }
      BIT_CLEAR(gpio,BF_MASK_GEN(GPIO_RAB_AR_LOG_CLR,1));
      BIT_CLEAR(gpio,BF_MASK_GEN(GPIO_RAB_AW_LOG_CLR,1));
      iowrite32(gpio, (void *)((unsigned long)(pulp->gpio)+0x8));
  
      // continue
      iowrite32(gpio_init, (void *)((unsigned long)(pulp->gpio)+0x8));
    }
  
    return;
  }
  // }}}

  // ax_log_print {{{
  /**
   * Print the content of the rab_ax_log_buf buffers and reset the incdices
   * rab_ax_log_idx.
   */
  void pulp_rab_ax_log_print(void)
  {
  
    unsigned i;
    unsigned addr, ts, meta;
    unsigned id, len;
    unsigned type;
  
    // read out and clear the AR log
    type = 0;
    for (i=0; i<rab_ar_log_buf_idx; i=i+3) {
      ts   = rab_ar_log_buf[i+0];
      meta = rab_ar_log_buf[i+1];
      addr = rab_ar_log_buf[i+2];
  
      len = BF_GET(meta, 0, 8 );
      id  = BF_GET(meta, 8, 10);
  
      #if RAB_AX_LOG_PRINT_FORMAT == 0 // DEBUG
        printk(KERN_INFO "AR Log: %u %#x %u %#x %u\n", ts, addr, len, id, type);
      #else // 1 = MATLAB
        printk(KERN_INFO "AR Log: %u %u %u %u %u\n", ts, addr, len, id, type);
      #endif
    }
    rab_ar_log_buf_idx = 0;
  
    // read out and clear the AW log
    type = 1;
    for (i=0; i<rab_aw_log_buf_idx; i=i+3) {
      ts   = rab_aw_log_buf[i+0];
      meta = rab_aw_log_buf[i+1];
      addr = rab_aw_log_buf[i+2];
  
      len = BF_GET(meta, 0, 8 );
      id  = BF_GET(meta, 8, 10);
  
      #if RAB_AX_LOG_PRINT_FORMAT == 0 // DEBUG
        printk(KERN_INFO "AW Log: %u %#x %u %#x %u\n", ts, addr, len, id, type);
      #else // 1 = MATLAB
        printk(KERN_INFO "AW Log: %u %u %u %u %u\n", ts, addr, len, id, type);
      #endif
    }
    rab_aw_log_buf_idx = 0;
  
    return;
  }
  // }}}

#endif
// }}}

// Profiling {{{
#if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
  /***********************************************************************************
   *
   * ██████╗ ██████╗  ██████╗ ███████╗██╗██╗     ███████╗
   * ██╔══██╗██╔══██╗██╔═══██╗██╔════╝██║██║     ██╔════╝
   * ██████╔╝██████╔╝██║   ██║█████╗  ██║██║     █████╗  
   * ██╔═══╝ ██╔══██╗██║   ██║██╔══╝  ██║██║     ██╔══╝  
   * ██║     ██║  ██║╚██████╔╝██║     ██║███████╗███████╗
   * ╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝╚══════╝
   * 
   ***********************************************************************************/                                             

  // prof_init {{{
  /**
   * Dynamically allocate buffers used to store profiling data and initialize index counters.
   */
  int pulp_rab_prof_init(void)
  {
    // allocate buffer memory
    n_cyc_buf_response       = (unsigned *)vmalloc(PROFILE_RAB_N_UPDATES*sizeof(unsigned));
    n_cyc_buf_update         = (unsigned *)vmalloc(PROFILE_RAB_N_UPDATES*sizeof(unsigned));
    n_cyc_buf_setup          = (unsigned *)vmalloc(PROFILE_RAB_N_REQUESTS*sizeof(unsigned));
    n_cyc_buf_cache_flush    = (unsigned *)vmalloc(PROFILE_RAB_N_ELEMENTS*sizeof(unsigned));
    n_cyc_buf_get_user_pages = (unsigned *)vmalloc(PROFILE_RAB_N_ELEMENTS*sizeof(unsigned));
    if ( (n_cyc_buf_response       == NULL) ||
         (n_cyc_buf_update         == NULL) ||
         (n_cyc_buf_setup          == NULL) ||
         (n_cyc_buf_cache_flush    == NULL) ||
         (n_cyc_buf_get_user_pages == NULL) ) {
      printk(KERN_WARNING "PULP - RAB: Could not allocate kernel memory for profiling buffers.\n");
      return -ENOMEM;
    }

    // initialize indices
    idx_buf_response       = 0;
    idx_buf_update         = 0;
    idx_buf_setup          = 0;
    idx_buf_cache_flush    = 0;
    idx_buf_get_user_pages = 0;

    // initialize accumulators
    n_cyc_tot_response       = 0;
    n_cyc_tot_update         = 0;
    n_cyc_tot_setup          = 0;
    n_cyc_tot_cache_flush    = 0;
    n_cyc_tot_get_user_pages = 0;

    #ifdef PROFILE_RAB_STR
      n_cyc_buf_cleanup = (unsigned *)vmalloc(PROFILE_RAB_N_REQUESTS*sizeof(unsigned));
      n_cyc_buf_map_sg  = (unsigned *)vmalloc(PROFILE_RAB_N_ELEMENTS*sizeof(unsigned));
      if ( (n_cyc_buf_cleanup == NULL) ||
           (n_cyc_buf_map_sg  == NULL) ) {
        printk(KERN_WARNING "PULP - RAB: Could not allocate kernel memory for profiling buffers.\n");
        return -ENOMEM;        
      }

      idx_buf_cleanup = 0;
      idx_buf_map_sg  = 0;

      n_cyc_tot_cleanup = 0;
      n_cyc_tot_map_sg  = 0;
      n_pages_setup     = 0;
      n_cleanups        = 0;
      n_slices_updated  = 0;
      n_updates         = 0;
    #endif

    #ifdef PROFILE_RAB_MH
      n_cyc_buf_schedule    = (unsigned *)vmalloc(PROFILE_RAB_N_UPDATES*sizeof(unsigned));
      if ( n_cyc_buf_schedule == NULL ) {
        printk(KERN_WARNING "PULP - RAB: Could not allocate kernel memory for profiling buffers.\n");
        return -ENOMEM;
      }

      idx_buf_schedule = 0;

      n_cyc_tot_schedule = 0;
      n_first_misses     = 0;
      n_misses           = 0;
    #endif

    return 0;
  }
  // }}}

  // prof_free {{{
  /**
   * Free dynamically allocated buffers used for profiling.
   */
  void pulp_rab_prof_free(void)
  {
    vfree(n_cyc_buf_response      ); 
    vfree(n_cyc_buf_update        ); 
    vfree(n_cyc_buf_setup         ); 
    vfree(n_cyc_buf_cache_flush   ); 
    vfree(n_cyc_buf_get_user_pages); 

    #ifdef PROFILE_RAB_STR
      vfree(n_cyc_buf_cleanup     ); 
      vfree(n_cyc_buf_map_sg      ); 
    #endif

    #ifdef PROFILE_RAB_MH
      vfree(n_cyc_buf_schedule    ); 
    #endif

    return;
  }
  // }}}

  // prof_print {{{
  /**
   * Print data collected during profiling and reset index counters
   */
  void pulp_rab_prof_print(void)
  {
    int i;
    
    // print buffers
    if ( idx_buf_response ) {
      printk(KERN_INFO "n_cyc_buf_response[i] = \n");
      for (i=0; i<idx_buf_response; i++) {
        printk(KERN_INFO "%d \n", n_cyc_buf_response[i]);
      }
    }
    #ifdef PROFILE_RAB_MH
      if ( idx_buf_schedule ) {
        printk(KERN_INFO "n_cyc_buf_schedule[i] = \n");
        for (i=0; i<idx_buf_schedule; i++) {
          printk(KERN_INFO "%d \n", n_cyc_buf_schedule[i]);
        }
      }
    #endif
    if ( idx_buf_update ) {
      printk(KERN_INFO "n_cyc_buf_update[i] = \n");
      for (i=0; i<idx_buf_update; i++) {
        printk(KERN_INFO "%d \n", n_cyc_buf_update[i]);
      }
    }
    if ( idx_buf_setup ) {
      printk(KERN_INFO "n_cyc_buf_setup[i] = \n");
      for (i=0; i<idx_buf_setup; i++) {
        printk(KERN_INFO "%d \n", n_cyc_buf_setup[i]);
      }
    }
    #ifdef PROFILE_RAB_STR
      if ( idx_buf_cleanup ) {
        printk(KERN_INFO "n_cyc_buf_cleanup[i] = \n");
        for (i=0; i<idx_buf_cleanup; i++) {
          printk(KERN_INFO "%d \n", n_cyc_buf_cleanup[i]);
        }
      }
    #endif
    if ( idx_buf_cache_flush ) {
      printk(KERN_INFO "n_cyc_buf_cache_flush[i] = \n");
      for (i=0; i<idx_buf_cache_flush; i++) {
        printk(KERN_INFO "%d \n", n_cyc_buf_cache_flush[i]);
      }
    }
    if ( idx_buf_get_user_pages ) {
      printk(KERN_INFO "n_cyc_buf_get_user_pages[i] = \n");
      for (i=0; i<idx_buf_get_user_pages; i++) {
        printk(KERN_INFO "%d \n", n_cyc_buf_get_user_pages[i]);
      }
    }
    #ifdef PROFILE_RAB_STR
      if ( idx_buf_map_sg ) {
        printk(KERN_INFO "n_cyc_buf_map_sg[i] = \n");
        for (i=0; i<idx_buf_map_sg; i++) {
          printk(KERN_INFO "%d \n", n_cyc_buf_map_sg[i]);
        }
      }
    #endif

    // update accumulators in shared memory 
    iowrite32(n_cyc_tot_response,       (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_RESPONSE_OFFSET_B));
    iowrite32(n_cyc_tot_update,         (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_UPDATE_OFFSET_B));
    iowrite32(n_cyc_tot_setup,          (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_SETUP_OFFSET_B));
    iowrite32(n_cyc_tot_cache_flush,    (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_CACHE_FLUSH_OFFSET_B));
    iowrite32(n_cyc_tot_get_user_pages, (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_GET_USER_PAGES_OFFSET_B));

    // reset indices
    idx_buf_response       = 0;
    idx_buf_update         = 0;
    idx_buf_setup          = 0;
    idx_buf_cache_flush    = 0;
    idx_buf_get_user_pages = 0;
    
    // reset accumulators
    n_cyc_tot_response       = 0;
    n_cyc_tot_update         = 0;
    n_cyc_tot_setup          = 0;
    n_cyc_tot_cache_flush    = 0;
    n_cyc_tot_get_user_pages = 0;

    #ifdef PROFILE_RAB_STR
      iowrite32(n_cyc_tot_map_sg,   (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_MAP_SG_OFFSET_B));
      iowrite32(n_cyc_tot_cleanup,  (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_CLEANUP_OFFSET_B));
      iowrite32(n_pages_setup,      (void *)((unsigned long)(pulp->l3_mem)+N_PAGES_SETUP_OFFSET_B));
      iowrite32(n_cleanups,         (void *)((unsigned long)(pulp->l3_mem)+N_CLEANUPS_OFFSET_B));
      iowrite32(n_slices_updated,   (void *)((unsigned long)(pulp->l3_mem)+N_SLICES_UPDATED_OFFSET_B));
      iowrite32(n_updates,          (void *)((unsigned long)(pulp->l3_mem)+N_UPDATES_OFFSET_B));

      idx_buf_map_sg    = 0;
      idx_buf_cleanup   = 0;
    
      n_cyc_tot_map_sg  = 0;
      n_cyc_tot_cleanup = 0;
      n_pages_setup     = 0;
      n_cleanups        = 0;
      n_slices_updated  = 0;
      n_updates         = 0;
    #endif

    #ifdef PROFILE_RAB_MH
      iowrite32(n_cyc_tot_schedule, (void *)((unsigned long)(pulp->l3_mem)+N_CYC_TOT_SCHEDULE_OFFSET_B));
      iowrite32(n_first_misses,     (void *)((unsigned long)(pulp->l3_mem)+N_FIRST_MISSES_OFFSET_B));
      iowrite32(n_misses,           (void *)((unsigned long)(pulp->l3_mem)+N_MISSES_OFFSET_B));

      idx_buf_schedule   = 0;

      n_cyc_tot_schedule = 0;
      n_first_misses     = 0;
      n_misses           = 0;
    #endif

    return;
  }
  // }}}

#endif
// }}}

// vim: ts=2 sw=2 sts=2 et foldmethod=marker tw=100
