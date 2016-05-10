#include "pulp_rab.h"
#include "pulp_mem.h" /* for cache invalidation */

// global variables
static        unsigned rab_slices[RAB_N_MAPPINGS*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES];
static struct page   **page_ptrs[RAB_N_MAPPINGS*RAB_N_PORTS*RAB_N_SLICES];
static        unsigned page_ptr_ref_cntrs[RAB_N_MAPPINGS*RAB_N_PORTS*RAB_N_SLICES];
static        unsigned rab_mappings[RAB_N_MAPPINGS*RAB_N_PORTS*RAB_N_SLICES*3];
static        unsigned rab_mapping_active;

static TlbL2_t tlbl2;

// functions

/**
 * Initialize the arrays the driver uses to manage the RAB.
 */
void pulp_rab_init()
{
  int i;

  // initialize the RAB slices table
  for (i=0;i<RAB_N_MAPPINGS*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;i++)
    rab_slices[i] = 0;
  // initialize the RAB pages and reference counter lists
  for (i=0;i<RAB_N_MAPPINGS*RAB_N_PORTS*RAB_N_SLICES;i++) {
    page_ptrs[i] = 0;
    page_ptr_ref_cntrs[i] = 0;
  }
  // initialize the RAB mappings table
  for (i=0;i<RAB_N_MAPPINGS*RAB_N_PORTS*RAB_N_SLICES*3;i++) {
    rab_mappings[i] = 0;
  }
  rab_mapping_active = 0;

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
  int off_ptrs;

  err = 0;
  off_ptrs = (int)(rab_slice_req->rab_mapping*RAB_N_PORTS*RAB_N_SLICES);
 
  // get field in page_ptrs array
  for (i=0;i<RAB_N_SLICES*RAB_N_PORTS;i++) {
    if ( page_ptr_ref_cntrs[off_ptrs+i] == 0 ) {
      rab_slice_req->page_ptr_idx = i;
      break;
    }
    if ( i == (RAB_N_SLICES*RAB_N_PORTS) ) {
      printk(KERN_INFO "PULP RAB: No slice available.\n");
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
 * date.
 */
int pulp_rab_slice_check(RabSliceReq *rab_slice_req)
{
  int expired;
  unsigned char date_exp_i;
  unsigned off_slices;

  expired = 0;
  off_slices = rab_slice_req->rab_mapping*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;

  if (DEBUG_LEVEL_RAB > 2) {
    printk(KERN_INFO "PULP - RAB: Mapping %d, Port %d, Slice %d: Testing.\n",
           rab_slice_req->rab_mapping, rab_slice_req->rab_port, rab_slice_req->rab_slice);
  }
  RAB_GET_DATE_EXP(date_exp_i,rab_slices[off_slices+rab_slice_req->rab_port*RAB_N_SLICES
                                         *RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH]);
  if ( (rab_slice_req->date_cur > date_exp_i) || // found an expired slice
       ((rab_slice_req->date_cur == 0) && ( RAB_MAX_DATE_MH == date_exp_i)) ) // wrap around in RAB miss handling mode
    expired = 1;
  
  return expired;
}

/**
 * Get a free RAB slice. Returns 0 on success, -EIO otherwise.
 *
 * @rab_slice_req: specifies the current date.
 */
int pulp_rab_slice_get(RabSliceReq *rab_slice_req)
{
  int err, i;
  err = 0;
  
  // get slice number
  for (i=0;i<RAB_N_SLICES;i++) {
    rab_slice_req->rab_slice = i;   

    if ( pulp_rab_slice_check(rab_slice_req) ) // found an expired slice
      break;
    else if (i == (RAB_N_SLICES-1) ) { // no slice free
      //      printk(KERN_INFO "PULP - RAB: No slice available.\n");
      //return -EIO;
      err = 1;
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
  num_free_slice = 0;
  
  // get slice number
  for (i=0;i<RAB_N_SLICES;i++) {
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
  unsigned off_slices, off_ptrs, off_mappings;
  struct page ** pages_old;

  off_slices = rab_slice_req->rab_mapping*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;
  off_ptrs  = rab_slice_req->rab_mapping*RAB_N_PORTS*RAB_N_SLICES;
  off_mappings = rab_slice_req->rab_mapping*RAB_N_PORTS*RAB_N_SLICES*3;

  if (DEBUG_LEVEL_RAB > 0) {
    printk(KERN_INFO "PULP - RAB: Mapping %d, Port %d, Slice %d: Freeing.\n",
           rab_slice_req->rab_mapping, rab_slice_req->rab_port, rab_slice_req->rab_slice);
  }

  // get old configuration of the selected slice
  page_ptr_idx_old = rab_slices[off_slices+rab_slice_req->rab_port*RAB_N_SLICES
                                *RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH+2];
  pages_old = page_ptrs[off_ptrs+page_ptr_idx_old];

  if (pages_old) { // not used for a constant mapping
    
    // deactivate the slice
    entry = 0x10*(rab_slice_req->rab_port*RAB_N_SLICES+rab_slice_req->rab_slice);
    if (rab_mapping_active == rab_slice_req->rab_mapping)
      iowrite32(0, (void *)((unsigned)rab_config+entry+0x1c));

    // determine pages to be unlocked
    entry = rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH+1;
    RAB_GET_PAGE_IDX_START(page_idx_start_old,rab_slices[off_slices+entry]);
    RAB_GET_PAGE_IDX_END(page_idx_end_old,rab_slices[off_slices+entry]);

    // unlock remapped pages and invalidate caches
    if ( !(rab_slice_req->flags & 0x2) &&   // do not unlock pages in striped mode until the last slice is freed
         !(rab_slice_req->flags & 0x4) ) {  // only unlock pages in multi-mapping rule when the last mapping is removed 
      for (i=page_idx_start_old;i<=page_idx_end_old;i++) {
        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP - RAB: Mapping %d, Port %d, Slice %d: Unlocking Page %d.\n",
                 rab_slice_req->rab_mapping, rab_slice_req->rab_port, rab_slice_req->rab_slice, i);
        }
        // invalidate caches --- invalidates entire pages only --- really needed?
        //pulp_mem_cache_inv(pages_old[i],0,PAGE_SIZE);
        // unlock
        if (!PageReserved(pages_old[i])) 
          SetPageDirty(pages_old[i]);
        page_cache_release(pages_old[i]);
      }
    }
    // lower reference counter
    page_ptr_ref_cntrs[off_ptrs+page_ptr_idx_old]--;
      
    // free memory if no more references exist
    if ( !page_ptr_ref_cntrs[off_ptrs+page_ptr_idx_old] ) {
      if  ( !(rab_slice_req->flags & 0x4) ) // only free pages ptr in multi-mapping rule when the last mapping is removed 
        kfree(pages_old);
      page_ptrs[off_ptrs+page_ptr_idx_old] = 0;
    }
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB: Number of references to pages pointer = %d.\n",
             page_ptr_ref_cntrs[off_ptrs+page_ptr_idx_old]);
    }
  }

  // delete entries in the slices table
  for (i=0;i<RAB_TABLE_WIDTH;i++) {
    rab_slices[off_slices+rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH
               +rab_slice_req->rab_slice*RAB_TABLE_WIDTH+i] = 0;
  }
  // delete entries in the mappings table
  for (i=0;i<3;i++) {
    rab_mappings[off_mappings+rab_slice_req->rab_port*RAB_N_SLICES*3
                 +rab_slice_req->rab_slice*3+i] = 0;
  }
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
  unsigned offset, entry, off_slices, off_ptrs, off_mappings, flags;

  off_slices = rab_slice_req->rab_mapping*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;
  off_ptrs  = rab_slice_req->rab_mapping*RAB_N_PORTS*RAB_N_SLICES;
  off_mappings = rab_slice_req->rab_mapping*RAB_N_PORTS*RAB_N_SLICES*3;
 
  // occupy the slice
  entry = rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH+0;
  RAB_SET_PROT(rab_slices[off_slices+entry],rab_slice_req->prot);
  RAB_SET_PORT(rab_slices[off_slices+entry],rab_slice_req->rab_port);
  RAB_SET_USE_ACP(rab_slices[off_slices+entry],rab_slice_req->use_acp);
  RAB_SET_DATE_EXP(rab_slices[off_slices+entry],rab_slice_req->date_exp);
  RAB_SET_DATE_CUR(rab_slices[off_slices+entry],rab_slice_req->date_cur);

  entry++;
  RAB_SET_PAGE_IDX_START(rab_slices[off_slices+entry], rab_slice_req->page_idx_start);
  RAB_SET_PAGE_IDX_END(rab_slices[off_slices+entry], rab_slice_req->page_idx_end);

  entry++;
  rab_slices[off_slices+entry] = rab_slice_req->page_ptr_idx;

  // for the first segment, check that the selected reference list
  // entry is really free = memory has properly been freed
  if ( page_ptr_ref_cntrs[off_ptrs+rab_slice_req->page_ptr_idx] & !rab_slice_req->page_idx_start 
       & !(rab_slice_req->flags & 0x2) ) { 
    printk(KERN_WARNING "PULP - RAB: Selected reference list entry not free. Number of references = %d.\n"
           , page_ptr_ref_cntrs[off_ptrs+rab_slice_req->page_ptr_idx]);
    return -EIO;
  }
  page_ptr_ref_cntrs[off_ptrs+rab_slice_req->page_ptr_idx]++;
  if (DEBUG_LEVEL_RAB > 0) {
    printk(KERN_INFO "PULP - RAB: Number of references to pages pointer = %d.\n",
           page_ptr_ref_cntrs[off_ptrs+rab_slice_req->page_ptr_idx]);
  }

  if (rab_slice_req->flags & 0x1) {
    page_ptrs[off_ptrs+rab_slice_req->page_ptr_idx] = 0;
  }
  else {
    page_ptrs[off_ptrs+rab_slice_req->page_ptr_idx] = pages;
  }

  // set up new slice in mappings table
  rab_mappings[off_mappings+rab_slice_req->rab_port*RAB_N_SLICES*3+rab_slice_req->rab_slice*3+0] 
    = rab_slice_req->addr_start;
  rab_mappings[off_mappings+rab_slice_req->rab_port*RAB_N_SLICES*3+rab_slice_req->rab_slice*3+1] 
    = rab_slice_req->addr_end;
  rab_mappings[off_mappings+rab_slice_req->rab_port*RAB_N_SLICES*3+rab_slice_req->rab_slice*3+2] 
    = rab_slice_req->addr_offset;

  if (DEBUG_LEVEL_RAB > 0)
    printk(KERN_INFO "PULP - RAB: Mapping %d, Port %d, Slice %d: Setting up.\n", 
           rab_slice_req->rab_mapping, rab_slice_req->rab_port, rab_slice_req->rab_slice);

  // set up new slice, configure the hardware
  if (rab_mapping_active == rab_slice_req->rab_mapping) {
    offset = 0x10*(rab_slice_req->rab_port*RAB_N_SLICES+rab_slice_req->rab_slice);
    
    RAB_SLICE_SET_FLAGS(flags, rab_slice_req->prot, rab_slice_req->use_acp);
    
    iowrite32(rab_slice_req->addr_start,  (void *)((unsigned)rab_config+offset+0x10));
    iowrite32(rab_slice_req->addr_end,    (void *)((unsigned)rab_config+offset+0x14));
    iowrite32(rab_slice_req->addr_offset, (void *)((unsigned)rab_config+offset+0x18));
    iowrite32(flags,                      (void *)((unsigned)rab_config+offset+0x1c));
 
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP - RAB: addr_start  %#x\n", rab_slice_req->addr_start);
      printk(KERN_INFO "PULP - RAB: addr_end    %#x\n", rab_slice_req->addr_end);
      printk(KERN_INFO "PULP - RAB: addr_offset %#x\n", rab_slice_req->addr_offset);
      printk(KERN_INFO "PULP - RAB: flags       %#x\n", flags);
    }
  }  

  return 0;  
}

/**
 * Switch the current RAB setup with another mapping.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @rab_mapping: specifies the mapping to set up the RAB with.
 */
void pulp_rab_switch_mapping(void *rab_config, unsigned rab_mapping)
{
  int i,j;
  unsigned offset, off_slices, off_mappings, flags;
  unsigned char prot, use_acp;

  if (DEBUG_LEVEL_RAB > 0)
    printk(KERN_INFO "PULP - RAB: Switch from Mapping %d to %d.\n",rab_mapping_active,rab_mapping);

  // de-activate old mapping
  off_slices = rab_mapping_active*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;
  for (i=0; i<RAB_N_PORTS; i++) {
    for (j=0; j<RAB_N_SLICES; j++) {
      RAB_GET_PROT(prot, rab_slices[off_slices+i*RAB_TABLE_WIDTH*RAB_N_SLICES+j*RAB_TABLE_WIDTH+0]);
      if (prot) { // de-activate slices with old active config

        offset = 0x10*(i*RAB_N_SLICES+j);
        iowrite32(0x0,(void *)((unsigned)rab_config+offset+0x1c));

        if (DEBUG_LEVEL_RAB > 0)
          printk(KERN_INFO "PULP - RAB: Mapping %d, Port %d, Slice %d: Disabling.\n",rab_mapping_active,i,j);
      }
    }
  }
  
  off_slices = rab_mapping*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;
  off_mappings = rab_mapping*RAB_N_PORTS*RAB_N_SLICES*3;
   
  // set up new mapping
  for (i=0; i<RAB_N_PORTS; i++) {
    for (j=0; j<RAB_N_SLICES; j++) {
      RAB_GET_PROT(prot, rab_slices[off_slices+i*RAB_TABLE_WIDTH*RAB_N_SLICES+j*RAB_TABLE_WIDTH+0]);
      if (prot & 0x1) { // activate slices with new active config
        offset =  0x10*(i*RAB_N_SLICES+j);

        RAB_GET_USE_ACP(use_acp, rab_slices[off_slices+i*RAB_TABLE_WIDTH*RAB_N_SLICES+j*RAB_TABLE_WIDTH+0]);
        RAB_SLICE_SET_FLAGS(flags, prot, use_acp);
    
        iowrite32(rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+0], (void *)((unsigned)rab_config+offset+0x10));
        iowrite32(rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+1], (void *)((unsigned)rab_config+offset+0x14));
        iowrite32(rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+2], (void *)((unsigned)rab_config+offset+0x18));
        iowrite32(flags, (void *)((unsigned)rab_config+offset+0x1c));
	
        if (DEBUG_LEVEL_RAB > 0)
          printk(KERN_INFO "PULP - RAB: Mapping %d, Port %d, Slice %d: Setting up.\n",rab_mapping,i,j);
        if (DEBUG_LEVEL_RAB > 1) {
          printk(KERN_INFO "PULP - RAB: addr_start  %#x\n", rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+0]);
          printk(KERN_INFO "PULP - RAB: addr_end    %#x\n", rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+1]);
          printk(KERN_INFO "PULP - RAB: addr_offset %#x\n", rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+2]);
        }
      }
    }
  }
 
  rab_mapping_active = rab_mapping;
}

/**
 * Print RAB mappings.
 *
 * @rab_config:  kernel virtual address of the RAB configuration port.
 * @rab_mapping: specifies the mapping to to print, 0xAAAA for actual
 *               RAB configuration, 0xFFFF for all mappings.
 */
void pulp_rab_print_mapping(void *rab_config, unsigned rab_mapping)
{
  int mapping_min, mapping_max;
  int i,j,k;
  unsigned offset, off_slices, off_mappings, off_ptrs;
  unsigned addr_start, addr_end, addr_offset, prot, use_acp, flags;

  if (rab_mapping == 0xFFFF) {
    mapping_min = 0;
    mapping_max = RAB_N_MAPPINGS;
  }
  else if (rab_mapping == 0xAAAA) {
    mapping_min = 0;
    mapping_max = 0;
  }
  else {
    mapping_min = rab_mapping;
    mapping_max = rab_mapping+1;
  }

  for (k=mapping_min; k<mapping_max; k++) {
    printk(KERN_INFO "PULP - RAB: Printing Mapping %d: \n",k);
    
    off_slices = k*RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;
    off_mappings = k*RAB_N_PORTS*RAB_N_SLICES*3;

    for (i=0; i<RAB_N_PORTS; i++) {
      for (j=0; j<RAB_N_SLICES; j++) {
        addr_start   = rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+0];
        addr_end     = rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+1];
        addr_offset  = rab_mappings[off_mappings+i*RAB_N_SLICES*3+j*3+2];
        RAB_GET_PROT(prot, rab_slices[off_slices+i*RAB_TABLE_WIDTH*RAB_N_SLICES
                                      +j*RAB_TABLE_WIDTH+0]);
        RAB_GET_USE_ACP(use_acp, rab_slices[off_slices+i*RAB_TABLE_WIDTH*RAB_N_SLICES
                                            +j*RAB_TABLE_WIDTH+0]);

        if (prot)
          printk(KERN_INFO "Port %d, Slice %d: %#x - %#x -> %#x , %#x, acp: %d\n",
                 i,j,addr_start,addr_end,addr_offset,prot,use_acp);

      }
    }
  } 

  if ( (rab_mapping ==  0xFFFF) || (rab_mapping == 0xAAAA) ) {
    printk(KERN_INFO "PULP - RAB: Printing active configuration: \n");
    
    for (i=0; i<RAB_N_PORTS; i++) {
      for (j=0; j<RAB_N_SLICES; j++) {
        offset = 0x10*(i*RAB_N_SLICES+j);
  
        flags = ioread32((void *)((unsigned)rab_config+offset+0x1C));
        RAB_SLICE_GET_FLAGS(flags, prot, use_acp);
        
        if (prot) {
          addr_start   = ioread32((void *)((unsigned)rab_config+offset+0x10));
          addr_end     = ioread32((void *)((unsigned)rab_config+offset+0x14));
          addr_offset  = ioread32((void *)((unsigned)rab_config+offset+0x18));
          
          printk(KERN_INFO "Port %d, Slice %d: %#x - %#x -> %#x , %#x, acp: %d\n",
                 i,j,addr_start,addr_end,addr_offset,prot,use_acp);
        }
      }
    }
  }

  // print the RAB pages and reference counter lists
  for (k=mapping_min; k<mapping_max; k++) {
    off_ptrs  = k*RAB_N_PORTS*RAB_N_SLICES;
    printk(KERN_INFO "PULP - RAB: Printing Mapping %d: \n",k);

    for (i=0; i<RAB_N_PORTS; i++) {
      for (j=0; j<RAB_N_SLICES; j++) {
        if (page_ptr_ref_cntrs[off_ptrs+i*RAB_N_SLICES+j])
          printk(KERN_INFO "Port %d, Slice %d: page_ptrs[i] = %#x, page_ptr_ref_cntrs[i] = %d\n",
                 i,j,(unsigned int)page_ptrs[off_ptrs+i*RAB_N_SLICES+j],
                 page_ptr_ref_cntrs[off_ptrs+i*RAB_N_SLICES+j]);
      }
    }
  }

}


/**
 * Initialise L2 TLB HW and struct to zero.
 *
 * @rab_config: kernel virtual address of the RAB configuration port.
 * @port: RAB port
 */
void pulp_l2tlb_init_zero(void *rab_config, char port)
{
  unsigned int set_num, entry_num, offset;
  
  for (set_num=0; set_num<TLBL2_NUM_SETS; set_num++) {
    for (entry_num=0; entry_num<TLBL2_NUM_ENTRIES_PER_SET; entry_num++) {
      // Clear VA ram. No need to clear PA ram.
      offset = ((port+1)*0x4000) + (set_num*TLBL2_NUM_ENTRIES_PER_SET*4) + (entry_num*4);
      iowrite32( 0, (void *)((unsigned)rab_config + offset)); 
      tlbl2.set[set_num].entry[entry_num].flags = 0;
      tlbl2.set[set_num].entry[entry_num].pfn_p = 0;
      tlbl2.set[set_num].entry[entry_num].pfn_v = 0;
    }
    tlbl2.set[set_num].next_entry_idx = 0;
    tlbl2.set[set_num].is_full = 0;
  }
  printk(KERN_INFO "PULP TLB: Initialized TLB RAMs to 0.\n");

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
int pulp_l2tlb_setup_entry(void *rab_config, TlbL2Entry_t *tlb_entry, char port, char enable_replace)
{
  unsigned set_num, entry_num;
  unsigned data_v, data_p, addr_v, addr_p;
  
  int err = 0;
  int full = 0;

  set_num = BF_GET(tlb_entry->pfn_v,0,5);
  entry_num = tlbl2.set[set_num].next_entry_idx;

  //Check if set is full
  full = pulp_l2tlb_check_availability(tlb_entry, port);
  if (full == 1 && enable_replace == 0){
    err = 1;
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP TLB: Set %d is full in L2TLB. Replace not allowed"
             ,set_num);
    }
  }
  if (full == 1 && enable_replace == 1){ 
    pulp_l2tlb_invalidate_entry(rab_config, port, set_num, entry_num);
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP: Removing L2 TLB entry  at set %d entry %d \n",
             set_num, entry_num);
    }
    full = 0;
  }


  if (full == 0) {
    //Set is not full. Issue a write.
    addr_v = ((unsigned)rab_config+((port+1)*0x4000)+(set_num*TLBL2_NUM_ENTRIES_PER_SET*4)+(entry_num*4));
    data_v = tlb_entry->flags;
    BF_SET(data_v, tlb_entry->pfn_v, 4, 20); // Parameterise TODO.
    iowrite32(data_v, (void *)(addr_v));
    addr_p = ((unsigned)rab_config+((port+1)*0x4000)+(set_num*TLBL2_NUM_ENTRIES_PER_SET*4)+(entry_num*4)+1024*4); // PA RAM address. Parameterise TODO.
    data_p =  tlb_entry->pfn_p;
    iowrite32(data_p, (void *)(addr_p));
    
    // Update kernel struct
    tlbl2.set[set_num].entry[entry_num] = *tlb_entry;
    tlbl2.set[set_num].next_entry_idx++;
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP: Done Writing L2 TLB entry. PFN_V= %#x, PFN_P = %#x, at set %d entry %d \n",
             tlb_entry->pfn_v, tlb_entry->pfn_p, set_num, entry_num);
    }
    if (tlbl2.set[set_num].next_entry_idx == TLBL2_NUM_ENTRIES_PER_SET) {
      tlbl2.set[set_num].is_full = 1;
      tlbl2.set[set_num].next_entry_idx = 0;
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
int pulp_l2tlb_check_availability(TlbL2Entry_t *tlb_entry, char port)
{
  unsigned set_num;
  int full = 0;

  set_num = BF_GET(tlb_entry->pfn_v,0,5);

  //Check if set is full
  if (tlbl2.set[set_num].is_full == 1) {
    full = 1;
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP TLB: Set %d is full in L2TLB. Cannot fill further without replacing."
             ,set_num);
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
int pulp_l2tlb_invalidate_all_entries(void *rab_config, char port)
{
  int set_num, entry_num;

  for (set_num=0; set_num<TLBL2_NUM_SETS; set_num++) {
    for (entry_num=0; entry_num<tlbl2.set[set_num].next_entry_idx; entry_num++) {
      pulp_l2tlb_invalidate_entry(rab_config, port, set_num, entry_num);
    }
    tlbl2.set[set_num].next_entry_idx = 0;
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
int pulp_l2tlb_invalidate_entry(void *rab_config, char port, int set_num, int entry_num)
{
  unsigned data;
  struct page * page_old;

  if (DEBUG_LEVEL_RAB > 1) {
    printk(KERN_INFO "PULP - RAB: Invalidating L2 TLB entry- Port %d, Set %d, Entry %d.\n",port,set_num, entry_num);
  }
  data = tlbl2.set[set_num].entry[entry_num].flags;
  data = data >> 1;
  data = data << 1; // LSB is now zero.
  tlbl2.set[set_num].entry[entry_num].flags = data; // Update kernel struct
  BF_SET(data, tlbl2.set[set_num].entry[entry_num].pfn_v, 4, 20); // Parameterise TODO.
  iowrite32( data, (void *)((unsigned)rab_config+((port+1)*0x4000) + 
                            (set_num*TLBL2_NUM_ENTRIES_PER_SET*4) + (entry_num*4)) );

  // unlock pages and invalidate cache.
  page_old = tlbl2.set[set_num].entry[entry_num].page_ptr;  
  if (!PageReserved(page_old)) 
    SetPageDirty(page_old);
  page_cache_release(page_old);  

  // free the allocated memory for page struct. TODO
  //kfree(tlbl2.set[set_num].entry[entry_num].page_ptr);

  return 0;
}

/**
 * Print all entries.
 *
 * @port: RAB port
 */
int pulp_l2tlb_print_all_entries(char port)
{
  int set_num, entry_num;
  printk(KERN_INFO "PULP L2TLB Mapping: Port %d\n", port);
  for (set_num=0; set_num<TLBL2_NUM_SETS; set_num++) {
    for (entry_num=0; entry_num<tlbl2.set[set_num].next_entry_idx; entry_num++) {
      printk(KERN_INFO "PULP L2TLB Mapping: Virtual pageframe num = %#x, Physical pageframe num=%#x, Flags=%#x\n",
             tlbl2.set[set_num].entry[entry_num].pfn_v,
             tlbl2.set[set_num].entry[entry_num].pfn_p,
             tlbl2.set[set_num].entry[entry_num].flags
             );
    }
  }
  return 0;
}

int pulp_l2tlb_print_valid_entries(char port)
{
  int set_num, entry_num;
  printk(KERN_INFO "PULP L2TLB Mapping: Port %d\n", port);
  for (set_num=0; set_num<TLBL2_NUM_SETS; set_num++) {
    for (entry_num=0; entry_num<tlbl2.set[set_num].next_entry_idx; entry_num++) {
      if (tlbl2.set[set_num].entry[entry_num].flags & 0x1) {
        printk(KERN_INFO "PULP L2TLB Mapping: Set=%d, Entry=%d, Virtual pageframe num = %#x, Physical pageframe num=%#x, Flags=%#x\n",
               set_num, entry_num,
               tlbl2.set[set_num].entry[entry_num].pfn_v,
               tlbl2.set[set_num].entry[entry_num].pfn_p,
               tlbl2.set[set_num].entry[entry_num].flags
               );
      }
    }
  }
  return 0;
}

//////// Other functions
// pulp_l2tlb_replace_entry()
// pulp_l2tlb_find_entry_to_replace()



// The user will give preference as L1 or L2. Default option is L1 . try to group all nearby L1 together and call ioctl, same as before.
// If all slices are used up, give back error for now. Later, we will make them fill L2 in case L1 is full.
// group all L2 together and call ioctl. There should be only one ioctl for all of L2.
// Further enhancement would be to use some algorithm to dynamically assign the mapping to L1 or L2 based on region size and frequency of occurance.

