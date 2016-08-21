#ifndef _PULP_RAB_H_
#define _PULP_RAB_H_

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* KERN_ALERT, container_of */
#include <linux/mm.h>      /* vm_area_struct struct, page struct, PAGE_SHIFT, pageo_phys */
#include <linux/pagemap.h> /* page_cache_release() */
#include <linux/slab.h>    /* kmalloc() */
#include <asm/io.h>        /* ioremap, iounmap, iowrite32 */

#include "pulp_module.h"

#include "pulp_host.h"

#define RAB_L2_N_ENTRIES_PER_SET 32
#define RAB_L2_N_SETS            32

// type definitions
typedef struct {
  // management
  unsigned char date_cur;
  unsigned char date_exp;
  unsigned char page_ptr_idx;
  unsigned char page_idx_start;
  unsigned char page_idx_end;
  unsigned char rab_port;  
  unsigned      rab_mapping;
  unsigned      rab_slice;
  unsigned char flags_drv; // bit 0 = const mapping, bit 1 = striped mapping, bit 2 = set up in every RAB mapping
  // actual config
  unsigned      addr_start;
  unsigned      addr_end;
  unsigned long addr_offset;
  unsigned char flags_hw;  // bits 0-2: prot, bit 3: use_acp
} RabSliceReq;

// Stripe request structs - kernel space 
typedef struct {
  unsigned      addr_start;
  unsigned      addr_end;
  unsigned long addr_offset;
} RabStripeSlice;

typedef struct {
  unsigned         n_slices;      // number of slices used for that stripe 
  RabStripeSlice * slice_configs; // ptr to array of slice configs
} RabStripe;

typedef struct {
  // management
  unsigned char id;
  unsigned char type;                // in = 2, out = 3, inout = 4
  unsigned char page_ptr_idx;
  unsigned      n_slices;            // number of slices allocated
  unsigned      n_slices_per_stripe; // number of slices used per stripe
  unsigned *    slice_idxs;          // ptr to array containing idxs of allocated slices 
  // actual config
  unsigned      stripe_idx;          // idx next stripe to configure
  unsigned      n_stripes; 
  RabStripe *   stripes;             // ptr to array of stripe structs
  unsigned char flags_hw;
} RabStripeElem;

typedef struct {
  unsigned short  id;
  unsigned short  n_elements;
  RabStripeElem * elements;
} RabStripeReq;

// L1 TLB structs
typedef struct {
  // management
  unsigned char date_cur;
  unsigned char date_exp;
  // actual config
  unsigned      addr_start;
  unsigned      addr_end;
  unsigned long addr_offset;
  unsigned char flags;
} L1EntryPort0;

typedef struct {
  L1EntryPort0 slices[RAB_L1_N_SLICES_PORT_0];
} L1TlbPort0;

typedef struct {
  // management
  unsigned char date_cur;
  unsigned char date_exp;
  unsigned char page_ptr_idx;
  unsigned char page_idx_start;
  unsigned char page_idx_end;
  // actual config
  unsigned      addr_start;
  unsigned      addr_end;
  unsigned long addr_offset;
  unsigned char flags;
} L1EntryPort1;

typedef struct {
  L1EntryPort1   slices            [RAB_L1_N_SLICES_PORT_1];
  struct page ** page_ptrs         [RAB_L1_N_SLICES_PORT_1];
  unsigned       page_ptr_ref_cntrs[RAB_L1_N_SLICES_PORT_1];
} L1TlbMappingPort1;

typedef struct {
  L1TlbMappingPort1 mappings[RAB_L1_N_MAPPINGS_PORT_1];
  unsigned          mapping_active;
} L1TlbPort1;

typedef struct {
  L1TlbPort0 port_0;
  L1TlbPort1 port_1;
} L1Tlb;

// L2 TLB structs
typedef struct {
  unsigned char flags; 
  unsigned      pfn_v; 
  unsigned      pfn_p; 
  struct page * page_ptr;   
} L2Entry;

typedef struct {
  L2Entry       entry[RAB_L2_N_ENTRIES_PER_SET];
  unsigned char next_entry_idx;
  unsigned char is_full;
} L2Set;

typedef struct {
  L2Set set[RAB_L2_N_SETS];
} L2Tlb;


// methods declarations
void pulp_rab_l1_init(void);

int  pulp_rab_page_ptrs_get_field(RabSliceReq *rab_slice_req);

int  pulp_rab_slice_check(RabSliceReq *rab_slice_req);
int  pulp_rab_slice_get(RabSliceReq *rab_slice_req);
void pulp_rab_slice_free(void *rab_config, RabSliceReq *rab_slice_req);
int  pulp_rab_slice_setup(void *rab_config, RabSliceReq *rab_slice_req, struct page **pages);

int  pulp_rab_mapping_get_active(void);
void pulp_rab_mapping_switch(void *rab_config, unsigned rab_mapping);
void pulp_rab_mapping_print(void *rab_config, unsigned rab_mapping);

void pulp_rab_l2_init(void *rab_config);

int pulp_rab_l2_setup_entry(void *rab_config, L2Entry *tlb_entry, char port, char enable_replace);
int pulp_rab_l2_check_availability(L2Entry *tlb_entry, char port);

int pulp_rab_l2_invalidate_all_entries(void *rab_config, char port);
int pulp_rab_l2_invalidate_entry(void *rab_config, char port, int set_num, int entry_num);
int pulp_rab_l2_print_all_entries(char port);
int pulp_rab_l2_print_valid_entries(char port);

int pulp_rab_num_free_slices(RabSliceReq *rab_slice_req);


#endif/*_PULP_RAB_H_*/
