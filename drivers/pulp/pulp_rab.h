#ifndef _PULP_RAB_H_
#define _PULP_RAB_H_

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* KERN_ALERT, container_of */
#include <linux/mm.h>           /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/pagemap.h>      /* page_cache_release() */
#include <linux/slab.h>         /* kmalloc() */
#include <asm/io.h>		/* ioremap, iounmap, iowrite32 */

#include "pulp_module.h"

#include "pulp_host.h"

#define RAB_TABLE_WIDTH 3

// macros
#define RAB_GET_PAGE_IDX_START(idx_start, page_idxs ) \
  ( idx_start = BF_GET( page_idxs, RAB_CONFIG_N_BITS_PAGE, RAB_CONFIG_N_BITS_PAGE))
#define RAB_SET_PAGE_IDX_START(page_idxs, idx_start) \
  ( BF_SET(page_idxs, idx_start, RAB_CONFIG_N_BITS_PAGE, RAB_CONFIG_N_BITS_PAGE) )

#define RAB_GET_PAGE_IDX_END(idx_end, page_idxs) \
  ( idx_end = BF_GET(page_idxs, 0, RAB_CONFIG_N_BITS_PAGE))
#define RAB_SET_PAGE_IDX_END(page_idxs, idx_end) \
  ( BF_SET(page_idxs, idx_end, 0, RAB_CONFIG_N_BITS_PAGE) )

// type definitions
typedef struct {
  unsigned rab_slice;
  unsigned char rab_port;
  unsigned char prot;
  unsigned char date_exp;
  unsigned char date_cur;
  unsigned addr_start;
  unsigned addr_end;
  unsigned addr_offset;
  unsigned page_ptr_idx;
  unsigned page_idx_start;
  unsigned page_idx_end;
  unsigned flags; // bit 0 = const mapping, bit 1 = striped mapping
} RabSliceReq;

typedef struct {
  unsigned n_slices;
  unsigned *slices;
  unsigned char rab_port;
  unsigned char prot;
  unsigned n_stripes;
  unsigned *rab_stripes;
  unsigned page_ptr_idx;
} RabStripeElem;

typedef struct {
  unsigned n_elements;
  RabStripeElem ** elements;
  unsigned n_stripes;
  unsigned stripe_idx;
} RabStripeReq;


// methods declarations
void pulp_rab_init(void);

int  pulp_rab_page_ptrs_get_field(RabSliceReq *rab_slice_req);

int  pulp_rab_slice_check(RabSliceReq *rab_slice_req);
int  pulp_rab_slice_get(RabSliceReq *rab_slice_req);
void pulp_rab_slice_free(void *rab_config, RabSliceReq *rab_slice_req);
int  pulp_rab_slice_setup(void *rab_config, RabSliceReq *rab_slice_req, struct page **pages);

#endif/*_PULP_RAB_H_*/
