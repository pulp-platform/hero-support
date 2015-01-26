#include "pulp_dma.h"

// global variables

// functions

/**
 * Request a channel of the PL330 DMA inside the Zynq PS.
 *
 * @chan: pointer to the dma_chan struct to use.
 * @chan_id: ID of the channel to request. 0: Host -> PULP, 1: PULP -> Host.
 */
int pulp_dma_chan_req(struct dma_chan ** chan, int chan_id)
{
  dma_cap_mask_t    mask;
  dmafilter_param_t filter;
  
  dma_cap_zero(mask);
  dma_cap_set(DMA_SLAVE, mask);

  filter.driver_name = "dma-pl330";
  filter.chan_id = chan_id;

  if (DEBUG_LEVEL_DMA > 1)
    filter.debug_print = 1;
  else
    filter.debug_print = 0;
  
  *chan = dma_request_channel(mask, dmafilter_fn, &filter);
  
  if (!(*chan)) {
    printk(KERN_WARNING "PULP - DMA: Channel request for %s[%d] failed.\n",
	   filter.driver_name, filter.chan_id);
    return -EBUSY;
  }

  return 0;
}

/**
 * Clean up a DMA channel through the Linux DMA API.
 *
 * @chan: pointer to the dma_chan struct.
 */
void pulp_dma_chan_clean(struct dma_chan * chan)
{
  dmaengine_terminate_all(chan);

  dma_release_channel(chan);
}

/**
 * Enqueue a new DMA transfer.
 *
 * @desc: pointer to the dma_async_tx_descriptor struct to fill.
 * @chan: pointer to the dma_chan struct to use.
 * @addr_dst: physical destination address.
 * @addr_src: physical source address.
 * @size_b: number of bytes to transfer.
 * @last: indicates the last transfer of series, set interrupt flag.
 */
int pulp_dma_xfer_prep(struct dma_async_tx_descriptor ** desc, struct dma_chan ** chan,
		       unsigned addr_dst, unsigned addr_src, unsigned size_b, bool last)
{
  unsigned flags;
  
  // set the interrupt flag for last transaction
  if (!last)
    flags = 0;
  else
    flags =  DMA_PREP_INTERRUPT;

  if (DEBUG_LEVEL_DMA > 1) {
    printk("PULP - DMA: New Segment.\n");
    printk("PULP - DMA: Source address      = %#x \n",addr_src);
    printk("PULP - DMA: Destination address = %#x \n",addr_dst);
    printk("PULP - DMA: Transfer size       = %#x \n",size_b);
  }

  // prepare the transaction
  *desc = (*chan)->device->device_prep_dma_memcpy(*chan, addr_dst, addr_src, size_b, flags);
  if ( *desc == NULL ) {
    printk("PULP - DMA: Transfer preparation failed.\n");
    return -EBUSY;
  }

  return 0;
}

/**
 * Clean up after a DMA transfer has finished, the callback
 * function. Unlocks user pages and frees memory.
 *
 * @pulp_dma_cleanup: pointer to the DmaCleanup struct.
 */
void pulp_dma_xfer_cleanup(DmaCleanup * pulp_dma_cleanup){

#ifndef PROFILE_DMA
  struct dma_async_tx_descriptor ** descs;
#endif

#ifdef PROFILE_DMA   
  struct page ** pages;
  unsigned n_pages;
  
  int i;
#endif

  if ( DEBUG_LEVEL_DMA > 0)
    printk("PULP - DMA: Transfer finished, cleanup called.\n");

  //// FIXME!!! - causes kernel panics if not in profiling mode
  //   Maybe a because pulp_dma_xfer_cleanup is executed as a tasklet!!!
#ifdef PROFILE_DMA
  // finally unlock remapped pages
  pages = *(pulp_dma_cleanup->pages);
  n_pages = pulp_dma_cleanup->n_pages;
  
  for (i=0; i<n_pages; i++) {
    if (!PageReserved(pages[i]))
      SetPageDirty(pages[i]);
    page_cache_release(pages[i]);
  }
  
  // free pages struct pointer array
  kfree(pages);
#endif
  ////

#ifndef PROFILE_DMA
  // terminate all transfers and free descriptors
  dmaengine_terminate_all(pulp_dma_cleanup->chan);

  // free transaction descriptors array
  descs = *(pulp_dma_cleanup->descs);
  kfree(descs);
#endif
  
  // signal to user-space runtime?
}

/**
 * Obtain a specific channel exclusively.  Used by the Linux DMA API,
 * provided by PL330 example of Norman Wong.
 *
 * @chan: pointer to the dma_chan struct to use.
 * @param: pointer to the filter parameters.
 */
bool dmafilter_fn(struct dma_chan *chan, void *param)
{
  struct dma_device    *device  = chan->device;
  struct device        *dev     = device->dev;
  struct device_driver *driver  = dev->driver;
  dmafilter_param_t    *filter  = param;
  int                   priv    = *((int *)(chan->private));

  if(filter->debug_print)
    printk("name=%s,chan_id=%d,private=%x\n",
           driver->name, chan->chan_id, priv);

  if(filter->driver_name && strcmp(driver->name, filter->driver_name))
    return false;

  if((filter->chan_id >= 0) && (chan->chan_id != filter->chan_id))
    return false;

  return(true);
}
