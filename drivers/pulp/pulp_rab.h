#ifndef _PULP_RAB_H_
#define _PULP_RAB_H_

#include <linux/module.h>	 /* Needed by all modules */
#include <linux/kernel.h>	 /* KERN_ALERT, container_of */
#include <linux/mm.h>      /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/pagemap.h> /* page_cache_release() */
#include <linux/slab.h>    /* kmalloc() */
#include <asm/io.h>		     /* ioremap, iounmap, iowrite32 */

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


// These macros can be used to extract the flags from a physical RAB slice in pulpemu
#define RAB_SLICE_SET_FLAGS(flags, prot, use_acp) \
  { BF_SET(flags, prot, 0, RAB_SLICE_FLAGS_PROT);                       \
    BF_SET(flags, use_acp, RAB_SLICE_FLAGS_PROT, RAB_SLICE_FLAGS_USE_ACP); }

#define RAB_SLICE_GET_FLAGS(flags, prot, use_acp) \
  { prot    = BF_GET(flags, 0, RAB_SLICE_FLAGS_PROT);                   \
    use_acp = BF_GET(flags, RAB_SLICE_FLAGS_PROT, RAB_SLICE_FLAGS_USE_ACP); }

// defines layout of the flags in a RAB slice
#define RAB_SLICE_FLAGS_PROT      3
#define RAB_SLICE_FLAGS_USE_ACP   1

// type definitions
typedef struct {
  unsigned rab_mapping;
  unsigned rab_slice;
  unsigned char rab_port;
  unsigned char use_acp;
  unsigned char prot;
  unsigned char date_exp;
  unsigned char date_cur;
  unsigned addr_start;
  unsigned addr_end;
  unsigned long addr_offset;
  unsigned page_ptr_idx;
  unsigned page_idx_start;
  unsigned page_idx_end;
  unsigned flags; // bit 0 = const mapping, bit 1 = striped mapping, bit 2 = set up in every mapping
} RabSliceReq;

typedef struct {
  unsigned rab_mapping;
  unsigned n_slices;
  unsigned n_slices_per_stripe;
  unsigned *slices;
  unsigned char rab_port;
  unsigned char use_acp;
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

void pulp_rab_switch_mapping(void *rab_config, unsigned rab_mapping);
void pulp_rab_print_mapping(void *rab_config, unsigned rab_mapping);

#endif/*_PULP_RAB_H_*/
