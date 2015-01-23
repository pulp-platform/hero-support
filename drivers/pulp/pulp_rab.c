#include "pulp_rab.h"

// global variables
static        unsigned rab_slices[RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES];
static struct page   **page_ptrs[RAB_N_PORTS*RAB_N_SLICES];
static        unsigned page_ptr_ref_cntrs[RAB_N_PORTS*RAB_N_SLICES];

// functions

/**
 * Initialize the arrays the driver uses to manage the RAB.
 */
void pulp_rab_init()
{
  int i;

  // initialize the RAB slices table
  for (i=0;i<RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;i++)
    rab_slices[i] = 0;
  // initialize the RAB pages and reference counter lists
  for (i=0;i<RAB_N_PORTS*RAB_N_SLICES;i++) {
    page_ptrs[i] = 0;
    page_ptr_ref_cntrs[i] = 0;
  }
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
  for (i=0;i<RAB_N_SLICES*RAB_N_PORTS;i++) {
    if ( page_ptr_ref_cntrs[i] == 0 ) {
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
  expired = 0;

  if (DEBUG_LEVEL_RAB > 1) {
    printk(KERN_INFO "PULP - RAB: Testing Slice %d on Port %d\n",
	   rab_slice_req->rab_slice, rab_slice_req->rab_port);
  }
  RAB_GET_DATE_EXP(date_exp_i,rab_slices[rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH
					 +rab_slice_req->rab_slice*RAB_TABLE_WIDTH]);
  if (rab_slice_req->date_cur > date_exp_i) // found an expired slice
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
      printk(KERN_INFO "PULP - RAB: No slice available.\n");
      return -EIO;
    }
  }

  return err;
}

/**
 * Free a RAB slice and unlock any corresponding memory pages.
 *
 * @rab_slice_req: specifies the slice to free.
 */
void pulp_rab_slice_free(RabSliceReq *rab_slice_req)
{
  int i;
  unsigned page_ptr_idx_old, page_idx_start_old, page_idx_end_old, entry;
  struct page ** pages_old;

  if (DEBUG_LEVEL_RAB > 0) {
    printk(KERN_INFO "PULP - RAB: Freeing Slice %d on Port %d.\n",
	   rab_slice_req->rab_slice, rab_slice_req->rab_port);
  }

  // get old configuration of the selected slice
  page_ptr_idx_old = rab_slices[rab_slice_req->rab_port*RAB_N_SLICES
				*RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH+2];
  pages_old = page_ptrs[page_ptr_idx_old];

  if (pages_old) { // not used for a constant mapping
    
    // determine pages to be unlocked
    entry = rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH+1;
    RAB_GET_PAGE_IDX_START(page_idx_start_old,rab_slices[entry]);
    RAB_GET_PAGE_IDX_END(page_idx_end_old,rab_slices[entry]);

    // unlock remapped pages
    for (i=page_idx_start_old;i<=page_idx_end_old;i++) {
      if (DEBUG_LEVEL_RAB > 0) {
	printk(KERN_INFO "PULP - RAB: Unlocking Page %d remapped on Slice %d on Port %d.\n",
	       i, rab_slice_req->rab_slice, rab_slice_req->rab_port);
      }
      if (!PageReserved(pages_old[i])) 
	SetPageDirty(pages_old[i]);
      page_cache_release(pages_old[i]);
      // lower reference counter
      page_ptr_ref_cntrs[page_ptr_idx_old]--;
    }

    // free memory if no more references exist
    if (!page_ptr_ref_cntrs[page_ptr_idx_old]) {
      kfree(pages_old);
      page_ptrs[page_ptr_idx_old] = 0;
    }
    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP - RAB: Number of references to pages pointer = %d.\n",
	     page_ptr_ref_cntrs[page_ptr_idx_old]);
    }
  }

  // delete entries in the table
  for (i=0;i<RAB_TABLE_WIDTH;i++) {
    rab_slices[rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH
	       +rab_slice_req->rab_slice*RAB_TABLE_WIDTH+i] = 0;
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
  unsigned offset, entry;
  
  // occupy the slice
  entry = rab_slice_req->rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice_req->rab_slice*RAB_TABLE_WIDTH+0;
  RAB_SET_PROT(rab_slices[entry],rab_slice_req->prot);
  RAB_SET_PORT(rab_slices[entry],rab_slice_req->rab_port);
  RAB_SET_DATE_EXP(rab_slices[entry],rab_slice_req->date_exp);
  RAB_SET_DATE_CUR(rab_slices[entry],rab_slice_req->date_cur);

  entry++;
  RAB_SET_PAGE_IDX_START(rab_slices[entry], rab_slice_req->page_idx_start);
  RAB_SET_PAGE_IDX_END(rab_slices[entry], rab_slice_req->page_idx_end);

  entry++;
  rab_slices[entry] = rab_slice_req->page_ptr_idx;

  // for the first segment, check that the selected reference list
  // entry is really free = memory has properly been freed
  if ( page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx] & !rab_slice_req->page_idx_start ) { 
    printk(KERN_WARNING "PULP - RAB: Selected reference list entry not free. Number of references = %d.\n"
	   , page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx]);
    return -EIO;
  }
  page_ptr_ref_cntrs[rab_slice_req->page_ptr_idx]++;
  
  if (rab_slice_req->const_mapping) {
    page_ptrs[rab_slice_req->page_ptr_idx] = 0;
  }
  else {
    page_ptrs[rab_slice_req->page_ptr_idx] = pages;
  }

  // setup new slice, configure the hardware
  offset = 0x10*(rab_slice_req->rab_port*RAB_N_SLICES+rab_slice_req->rab_slice);
  iowrite32(rab_slice_req->addr_start, rab_config+offset+0x10);
  iowrite32(rab_slice_req->addr_end, rab_config+offset+0x14);
  iowrite32(rab_slice_req->addr_offset, rab_config+offset+0x18);
  iowrite32(rab_slice_req->prot, rab_config+offset+0x1c);
 
  if (DEBUG_LEVEL_RAB > 0) {
  printk(KERN_INFO "PULP - RAB: Setting up Slice %d on Port %d.\n", 
	 rab_slice_req->rab_slice, rab_slice_req->rab_port);
  }
  if (DEBUG_LEVEL_RAB > 1) {
    printk(KERN_INFO "PULP - RAB: addr_start  %#x\n", rab_slice_req->addr_start);
    printk(KERN_INFO "PULP - RAB: addr_end    %#x\n", rab_slice_req->addr_end);
    printk(KERN_INFO "PULP - RAB: addr_offset %#x\n", rab_slice_req->addr_offset);
  }
  
  return 0;  
}
