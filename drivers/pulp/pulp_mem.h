#ifndef _PULP_MEM_H_
#define _PULP_MEM_H_

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* KERN_ALERT, container_of */
#include <linux/slab.h>         /* kmalloc */
#include <linux/mm.h>           /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/highmem.h>      /* kmap, kunmap */
#include <asm/cacheflush.h>     /* __cpuc_flush_dcache_area, outer_cache.flush_range */

#include "pulp_module.h"

#include "pulp_host.h"

// funtions
void pulp_mem_cache_flush(struct page * page, unsigned offset_start, unsigned offset_end);
unsigned  pulp_mem_get_num_pages(unsigned addr_start, unsigned size_b);
int pulp_mem_get_user_pages(struct page *** pages, unsigned addr_start, unsigned n_pages, unsigned write);
int pulp_mem_map_sg(unsigned ** addr_start_vec, unsigned ** addr_end_vec, unsigned ** addr_offset_vec,
		    unsigned ** page_start_idxs, unsigned ** page_end_idxs, 
		    struct page *** pages, unsigned n_pages, 
		    unsigned addr_start, unsigned addr_end);


#endif/*_PULP_MEM_H_*/
