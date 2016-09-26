#include "pulp_rab.h"
#include "pulp_mem.h" /* for cache invalidation */

// global variables

// L1 TLB
static L1Tlb l1;

// L2 TLB
static L2Tlb l2;

// functions

/**
 * Initialize the arrays the driver uses to manage the RAB.
 */
void pulp_rab_l1_init()
{
  int i,j;

  // Port 0
  for (i=0; i<RAB_L1_N_SLICES_PORT_0; i++) {
    l1.port_0.slices[i].date_cur       = 0;
    l1.port_0.slices[i].date_exp       = 0;
    l1.port_0.slices[i].addr_start     = 0;
    l1.port_0.slices[i].addr_end       = 0;
    l1.port_0.slices[i].addr_offset    = 0;
    l1.port_0.slices[i].flags          = 0;
  }

  // Port 1
  for (i=0; i<RAB_L1_N_MAPPINGS_PORT_1; i++) {
    for (j=0; j<RAB_L1_N_SLICES_PORT_1; j++) {
      l1.port_1.mappings[i].slices[j].date_cur       = 0;
      l1.port_1.mappings[i].slices[j].date_exp       = 0;
      l1.port_1.mappings[i].slices[j].page_ptr_idx   = 0;
      l1.port_1.mappings[i].slices[j].page_idx_start = 0;
      l1.port_1.mappings[i].slices[j].page_idx_end   = 0;
      l1.port_1.mappings[i].slices[j].addr_start     = 0;
      l1.port_1.mappings[i].slices[j].addr_end       = 0;
      l1.port_1.mappings[i].slices[j].addr_offset    = 0;
      l1.port_1.mappings[i].slices[j].flags          = 0;
      l1.port_1.mappings[i].page_ptrs[j]             = 0;
      l1.port_1.mappings[i].page_ptr_ref_cntrs[j]    = 0;
    }
  }
  l1.port_1.mapping_active = 0;

  return;
}

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
    if ( i == (RAB_L1_N_SLICES_PORT_1) ) {
      printk(KERN_INFO "PULP RAB L1: No page_ptrs field available on Port 1.\n");
      return -EIO;
    }
  }

  return err;
}

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

/**
 * Free a RAB slice and unlock any corresponding memory pages.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @rab_slice_req: specifies the slice to free.
 */
void pulp_rab_slice_free(void *rab_config, RabSliceReq *rab_slice_req)
{
  int i;
  unsigned page_ptr_idx_old, page_idx_start_old, page_idx_end_old, entry;
  unsigned port, mapping, slice;
  struct page ** pages_old;

  port    = rab_slice_req->rab_port;
  mapping = rab_slice_req->rab_mapping;
  slice   = rab_slice_req->rab_slice;

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
    if ( !(rab_slice_req->flags_drv & 0x2) &&   // do not unlock pages in striped mode until the last slice is freed
         !(rab_slice_req->flags_drv & 0x4) ) {  // only unlock pages in multi-mapping rule when the last mapping is removed 
      for (i=page_idx_start_old;i<=page_idx_end_old;i++) {
        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Unlocking Page %d.\n",
            port, mapping, slice, i);
        }
        // invalidate caches --- invalidates entire pages only --- really needed?
        pulp_mem_cache_inv(pages_old[i],0,PAGE_SIZE);
        // unlock
        if (!PageReserved(pages_old[i])) 
          SetPageDirty(pages_old[i]);
        page_cache_release(pages_old[i]);
      }
    }
    // lower reference counter
    l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old]--;
      
    // free memory if no more references exist
    if ( !l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old] ) {
      if  ( !(rab_slice_req->flags_drv & 0x4) ) // only free pages ptr in multi-mapping rule when the last mapping is removed 
        kfree(pages_old);
      l1.port_1.mappings[mapping].page_ptrs[page_ptr_idx_old] = 0;
    }
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB L1: Number of references to pages pointer = %d.\n", 
        l1.port_1.mappings[mapping].page_ptr_ref_cntrs[page_ptr_idx_old]);
    }
  }

  // clear entries in management structs
  if (port == 0) {
    l1.port_0.slices[slice].date_cur       = 0;
    l1.port_0.slices[slice].date_exp       = 0;
    l1.port_0.slices[slice].addr_start     = 0;
    l1.port_0.slices[slice].addr_end       = 0;
    l1.port_0.slices[slice].addr_offset    = 0;
    l1.port_0.slices[slice].flags          = 0;
  }
  else { // port 1
    l1.port_1.mappings[mapping].slices[slice].date_cur       = 0;
    l1.port_1.mappings[mapping].slices[slice].date_exp       = 0;
    l1.port_1.mappings[mapping].slices[slice].page_ptr_idx   = 0;
    l1.port_1.mappings[mapping].slices[slice].page_idx_start = 0;
    l1.port_1.mappings[mapping].slices[slice].page_idx_end   = 0;
    l1.port_1.mappings[mapping].slices[slice].addr_start     = 0;
    l1.port_1.mappings[mapping].slices[slice].addr_end       = 0;
    l1.port_1.mappings[mapping].slices[slice].addr_offset    = 0;
    l1.port_1.mappings[mapping].slices[slice].flags          = 0;
  }

  return;
}

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
    l1.port_0.slices[slice].flags       = rab_slice_req->flags_hw;
  }
  else { // Port 1
    l1.port_1.mappings[mapping].slices[slice].date_cur       = rab_slice_req->date_cur;
    l1.port_1.mappings[mapping].slices[slice].date_exp       = rab_slice_req->date_exp;
    l1.port_1.mappings[mapping].slices[slice].page_ptr_idx   = rab_slice_req->page_ptr_idx;
    l1.port_1.mappings[mapping].slices[slice].page_idx_start = rab_slice_req->page_idx_start;
    l1.port_1.mappings[mapping].slices[slice].page_idx_end   = rab_slice_req->page_idx_end;

    l1.port_1.mappings[mapping].slices[slice].addr_start  = rab_slice_req->addr_start;
    l1.port_1.mappings[mapping].slices[slice].addr_end    = rab_slice_req->addr_end;
    l1.port_1.mappings[mapping].slices[slice].addr_offset = rab_slice_req->addr_offset;
    l1.port_1.mappings[mapping].slices[slice].flags       = rab_slice_req->flags_hw;
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
      printk(KERN_INFO "PULP - RAB L1: flags       %#x\n",  rab_slice_req->flags_hw   );
    }
  }  

  return 0;  
}

/**
 * Get the index of the currently active RAB mapping on Port 1.
 */
int pulp_rab_mapping_get_active()
{
  return (int)l1.port_1.mapping_active;
}

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
    RAB_GET_PROT(prot, l1.port_1.mappings[mapping_active].slices[i].flags);
    if (prot) { // de-activate slices with old active config

      offset = 0x20*(1*RAB_L1_N_SLICES_PORT_0+i);
      iowrite32(0x0,(void *)((unsigned long)rab_config+offset+0x38));

      if (DEBUG_LEVEL_RAB > 0)
        printk(KERN_INFO "PULP - RAB L1: Port %d, Mapping %d, Slice %d: Disabling.\n",1,mapping_active,i);
    }
  }

  // set up new mapping
  for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
    RAB_GET_PROT(prot, l1.port_1.mappings[rab_mapping].slices[i].flags);
    if (prot & 0x1) { // activate slices with new active config

      offset = 0x20*(1*RAB_L1_N_SLICES_PORT_0+i);
  
      iowrite32(l1.port_1.mappings[rab_mapping].slices[i].addr_start,  (void *)((unsigned long)rab_config+offset+0x20));
      iowrite32(l1.port_1.mappings[rab_mapping].slices[i].addr_end,    (void *)((unsigned long)rab_config+offset+0x28));
      IOWRITE_L(l1.port_1.mappings[rab_mapping].slices[i].addr_offset, (void *)((unsigned long)rab_config+offset+0x30));
      iowrite32(l1.port_1.mappings[rab_mapping].slices[i].flags,       (void *)((unsigned long)rab_config+offset+0x38));

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
  unsigned offset, prot, flags, n_slices, page_ptr_idx;

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
      RAB_GET_PROT(prot, l1.port_1.mappings[rab_mapping].slices[i].flags);
      if (prot) {
        printk(KERN_INFO "Port %d, Slice %2d: %#x - %#x -> %#lx , flags = %#x\n", 1, i,
          l1.port_1.mappings[j].slices[i].addr_start,
          l1.port_1.mappings[j].slices[i].addr_end,
          l1.port_1.mappings[j].slices[i].addr_offset,
          l1.port_1.mappings[j].slices[i].flags);
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

        flags = ioread32((void *)((unsigned long)rab_config+offset+0x38));
        RAB_GET_PROT(prot, flags);
        if (prot) {
          printk(KERN_INFO "Port %d, Slice %2d: %#x - %#x -> %#lx , flags = %#x\n", j, i,
            ioread32((void *)((unsigned long)rab_config+offset+0x20)),
            ioread32((void *)((unsigned long)rab_config+offset+0x28)),
            (unsigned long)IOREAD_L((void *)((unsigned long)rab_config+offset+0x30)),
            flags);  
        }
      }
    }
  } 

  // print the RAB pages and reference counter lists
  for (j=mapping_min; j<mapping_max; j++) {
    printk(KERN_INFO "PULP - RAB L1: Printing Mapping %d: \n",j);

    for (i=0; i<RAB_L1_N_SLICES_PORT_1; i++) {
      RAB_GET_PROT(prot, l1.port_1.mappings[rab_mapping].slices[i].flags);
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
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB L2: Port %d, Set %d: Full.\n", port, set_num);
    }
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

//////// Other functions
// pulp_rab_l2_replace_entry()
// pulp_rab_l2_find_entry_to_replace()



// The user will give preference as L1 or L2. Default option is L1 . try to group all nearby L1 together and call ioctl, same as before.
// If all slices are used up, give back error for now. Later, we will make them fill L2 in case L1 is full.
// group all L2 together and call ioctl. There should be only one ioctl for all of L2.
// Further enhancement would be to use some algorithm to dynamically assign the mapping to L1 or L2 based on region size and frequency of occurance.

