#include "pulp_mem.h"

/**
 * Flush user-space memory pages.
 *
 * @page: pointer to the page struct of user-space memory page to flush.
 * @offset_start: address offset to start flushing from.
 * @offset_end: address offset to end flushing at.
 */
void pulp_mem_cache_flush(struct page *page, unsigned offset_start, unsigned offset_end) 
{

  void * addr;
  unsigned size_b;

  /* create a kernel-space mapping, the cache maintenance functions
     work on kernel virtual addresses, only exist for low memory */
  addr = kmap(page); 
  
  addr = addr + offset_start;
  size_b = offset_end - offset_start;

  // clean L1 cache lines
  __cpuc_flush_dcache_area(addr,size_b);
  
  // clean L2 cache lines 
  outer_cache.flush_range((long unsigned int)addr,size_b);

  // destroy kernel-space mapping
  kunmap(page);

}

/**
 * Get the number of pages to remap.
 */
unsigned pulp_mem_get_num_pages(unsigned addr_start, unsigned size_b) 
{
  unsigned n_pages;

  n_pages = (BF_GET(addr_start,0,PAGE_SHIFT) + size_b) >> PAGE_SHIFT; 
  if (BF_GET(BF_GET(addr_start,0,PAGE_SHIFT) + size_b,0,PAGE_SHIFT)) n_pages++; // remainder
  
  if (DEBUG_LEVEL_MEM > 1) {
    printk(KERN_INFO "PULP - MEM: Number of pages = %d\n",n_pages);
  }

  return n_pages;
}

/**
 * Get pointers to user-space pages' page structs of the calling
 * process and lock the corresponding pages in memory.
 *
 * @pages: pointer to the pages' page structs.
 * @addr_start: virtual user-space address to start remapping from.
 * @n_pages: number of pages to remap.
 * @write: if non-zero, remap pages in write mode.
 */
int pulp_mem_get_user_pages(struct page *** pages, unsigned addr_start, unsigned n_pages, unsigned write)
{
  int result;
  unsigned long start;

  // what get_user_pages returns
  *pages = (struct page **)kmalloc((size_t)(n_pages*sizeof(struct page *)),GFP_KERNEL);
  if (*pages == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }

  // align to page border / 4kB
  start = (unsigned long)(addr_start & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT));

  // get pointers to user-space buffers and lock them into memory
  down_read(&current->mm->mmap_sem);
  result = get_user_pages(current, current->mm, start, n_pages, write, 0, *pages, NULL);
  up_read(&current->mm->mmap_sem);
  if (result != n_pages) {
    printk(KERN_WARNING "PULP - MEM: Could not get requested user-space virtual addresses.\n");
    printk(KERN_WARNING "Requested %d pages starting at v_addr %#x\n",n_pages,addr_start);
    printk(KERN_WARNING "Obtained  %d pages\n",result);
  }

  return 0;
}

/**
 * Translates user-space pages' virtual addresses into physical
 * addresses and groups them into segments of physically contiguous
 * addresses. Returns the number of segments in case of success,
 * -ENOMEM otherwise.
 *
 * @addr_start_vec: pointer to array containing virtual start addresses of the segments.
 * @addr_end_vec: pointer to array containing virtual end addresses of the segments.
 * @addr_offset_vec: pointer to array containing address offsets (physical) of the segments.
 * @page_start_idxs: pointer to array containing start indexes of the segments in pages.
 * @page_end_idxs: pointer to array containing end indexes of the segments in pages.
 * @pages: pointer to the pages' page structs
 * @n_pages: number of pages to map.
 * @addr_start: virtual start address of the mapping.
 * @addr_end: virtual end address of the mapping.
 */
int pulp_mem_map_sg(unsigned ** addr_start_vec, unsigned ** addr_end_vec, unsigned ** addr_offset_vec,
		    unsigned ** page_start_idxs, unsigned ** page_end_idxs, 
		    struct page *** pages, unsigned n_pages, 
		    unsigned addr_start, unsigned addr_end)
{
  int i;
  unsigned n_segments;
  unsigned * addr_phys_vec;

  // virtual to physical address translation
  addr_phys_vec = (unsigned *)kmalloc((size_t)n_pages*sizeof(unsigned),GFP_KERNEL);
  if (addr_phys_vec == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }
  for (i=0;i<n_pages;i++) {
    addr_phys_vec[i] = (unsigned)page_to_phys((*pages)[i]);
    if (DEBUG_LEVEL_MEM > 1) {
      printk(KERN_INFO "PULP - MEM: Physical address = %#x\n",addr_phys_vec[i]);
    }
  }

  // setup mapping information
  *addr_start_vec = (unsigned *)kmalloc((size_t)(n_pages*sizeof(unsigned)),GFP_KERNEL);
  if (*addr_start_vec == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }
  *addr_end_vec = (unsigned *)kmalloc((size_t)(n_pages*sizeof(unsigned)),GFP_KERNEL);
  if (*addr_end_vec == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }
  *addr_offset_vec = (unsigned *)kmalloc((size_t)(n_pages*sizeof(unsigned)),GFP_KERNEL);
  if (*addr_offset_vec == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }
  *page_start_idxs = (unsigned *)kmalloc((size_t)(n_pages*sizeof(unsigned)),GFP_KERNEL);
  if (*page_start_idxs == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }
  *page_end_idxs = (unsigned *)kmalloc((size_t)(n_pages*sizeof(unsigned)),GFP_KERNEL);
  if (*page_end_idxs == NULL) {
    printk(KERN_WARNING "PULP - MEM: Memory allocation failed.\n");
    return -ENOMEM;
  }
  
  // initialization
  (*addr_start_vec)[0] = addr_start;
  (*addr_end_vec)[0] = addr_end;
  (*page_start_idxs)[0] = 0;
  (*page_end_idxs)[0] = 0;

  n_segments = 1;

  /*
   *  analyze the physical addresses
   */
  if ( n_pages > 1 ) {
     	
    // check the number of slices required
    for (i=1; i<n_pages; i++) {	  

      // are the pages also contiguous in physical memory?
      if (addr_phys_vec[i] != (addr_phys_vec[i-1] + PAGE_SIZE)) { // no

	if (DEBUG_LEVEL_MEM > 2) {
	  printk(KERN_INFO "PULP - MEM: Checking Page %d:\n",i);
	  printk(KERN_INFO "PULP - MEM: Page %d physical address = %#x\n",i,addr_phys_vec[i]);
	  printk(KERN_INFO "PULP - MEM: Page %d physical address = %#x\n",i-1,addr_phys_vec[i-1]);    
	}

	// finish current slice
	(*addr_end_vec)[n_segments-1] = (addr_start & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT) ) + PAGE_SIZE*i;
	(*page_end_idxs)[n_segments-1] = i-1;

	// add a new slice
	n_segments++;
	(*addr_start_vec)[n_segments-1] = (addr_start & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT) ) + PAGE_SIZE*i;
	(*addr_offset_vec)[n_segments-1] = addr_phys_vec[i];
	(*page_start_idxs)[n_segments-1] = i;
      }
      if (i == (n_pages-1)) {
	// finish last slice
	(*addr_end_vec)[n_segments-1] = addr_end;
	(*page_end_idxs)[n_segments-1] = i;
      }
    }
  }
  // set page offset
  (*addr_offset_vec)[0] = addr_phys_vec[0] + (addr_start & BIT_MASK_GEN(PAGE_SHIFT));
  // free memory
  kfree(addr_phys_vec);

  if (DEBUG_LEVEL_MEM > 1) {
    printk(KERN_INFO "PULP - MEM: Number of segments = %d\n",n_segments);
    for (i=0; i<n_segments; i++) {
      printk(KERN_INFO "PULP - MEM: Segment %d virtual start address  = %#x\n",i,(*addr_start_vec)[i]);
      printk(KERN_INFO "PULP - MEM: Segment %d virtual end address    = %#x\n",i,(*addr_end_vec)[i]);
      printk(KERN_INFO "PULP - MEM: Segment %d physical start address = %#x\n",i,(*addr_offset_vec)[i]);
    }
  }

  return n_segments;
}

