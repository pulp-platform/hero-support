#ifndef _PULP_DMA_H_
#define _PULP_DMA_H_

#include <linux/dmaengine.h>    /* dma_cap_zero(), dma_cap_set */
#include <linux/dma-mapping.h>  /* dma_alloc_coherent(),dma_free_coherent() */
#include <linux/completion.h>   /* For complete(), wait_for_completion_*() */
#include <linux/mm.h>           /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/pagemap.h>      /* page_cache_release() */
#include <linux/slab.h>         /* kmalloc() */

#include "pulp_module.h"

// type definitions
typedef struct {
  struct dma_chan * chan;
  struct dma_async_tx_descriptor *** descs;
  struct page *** pages;
  unsigned n_pages;
} DmaCleanup;

typedef struct {
  int         chan_id;     /* Set to negative to not use. */
  const char *driver_name; /* Set to NULL to not use. */
  int         debug_print;
} dmafilter_param_t;

// methods declarations
int pulp_dma_chan_req(struct dma_chan ** chan, int chan_id);
void pulp_dma_chan_clean(struct dma_chan * chan);

int pulp_dma_xfer_prep(struct dma_async_tx_descriptor ** desc, struct dma_chan ** chan,
		       unsigned addr_dst, unsigned addr_src, unsigned size_b, bool last);

void pulp_dma_xfer_cleanup(DmaCleanup * pulp_dma_cleanup);

bool dmafilter_fn(struct dma_chan *chan, void *param);
void callback_test(void);

#endif/*_PULP_DMA_H_*/
