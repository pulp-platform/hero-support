#include "pulp_mbox.h"

// global variables
static void * pulp_mbox;
static unsigned mbox_fifo[MBOX_FIFO_DEPTH*2];
static unsigned mbox_fifo_full = 0;
static unsigned * mbox_fifo_rd = mbox_fifo;
static unsigned * mbox_fifo_wr = mbox_fifo;
DEFINE_SPINLOCK(mbox_fifo_lock);
DECLARE_WAIT_QUEUE_HEAD(mbox_wq);

// functions

/**
 * Initialize the mailbox
 *
 * @mbox: kernel virtual address of the mailbox PULP-2-Host interface
 * (Interface 1) 
 */
void pulp_mbox_init(void *mbox)
{
  // initialize the pointer for pulp_mbox_read
  pulp_mbox = mbox;

  pulp_mbox_clear();

  return;
}

/**
 * Empty mbox_fifo buffer
 */
void pulp_mbox_clear(void)
{
  int i;

  // empty mbox_fifo
  if ( (mbox_fifo_wr != mbox_fifo_rd) || mbox_fifo_full) {
    for (i=0; i<2*MBOX_FIFO_DEPTH; i++) {
      printk(KERN_INFO "mbox_fifo[%d]: %d %d %#x \n",i,
        (&mbox_fifo[i] == mbox_fifo_wr) ? 1:0,
        (&mbox_fifo[i] == mbox_fifo_rd) ? 1:0,
        mbox_fifo[i]);
    }
  }
  mbox_fifo_wr = mbox_fifo;
  mbox_fifo_rd = mbox_fifo;
  mbox_fifo_full = 0;

  return;
}

/**
 * Read from PULP-2-Host mailbox interface to software FIFO
 * 
 * Bottom-half -> should actually go into a tasklet
 *
 * @mbox: kernel virtual address of the mailbox PULP-2-Host interface
 * (Interface 1) 
 */
void pulp_mbox_intr(void *mbox)
{
  int i;
  unsigned read, mbox_is, mbox_data;
  struct timeval time;
  
  if (DEBUG_LEVEL_MBOX > 0) {
    printk(KERN_INFO "PULP: Mailbox interrupt.\n");
  }
  
  // check interrupt status
  mbox_is = 0x7 & ioread32((void *)((unsigned long)mbox+MBOX_IS_OFFSET_B));
  
  if (mbox_is & 0x2) { // mailbox receive threshold interrupt
    // clear the interrupt
    iowrite32(0x2,(void *)((unsigned long)mbox+MBOX_IS_OFFSET_B));
  
    read = 0;
  
    spin_lock(&mbox_fifo_lock);
    // while mailbox not empty and FIFO buffer not full
    while ( !(0x1 & ioread32((void *)((unsigned long)mbox+MBOX_STATUS_OFFSET_B)))
            && !mbox_fifo_full ) {
      spin_unlock(&mbox_fifo_lock);
  
      // read mailbox
      mbox_data = ioread32((void *)((unsigned long)mbox+MBOX_RDDATA_OFFSET_B));
  
      if (mbox_data == RAB_UPDATE) {
        pulp_rab_update();
      }
      else if (mbox_data == RAB_SWITCH) {
        pulp_rab_switch();
      }
      else { // read to mailbox FIFO buffer
        if (mbox_data == TO_RUNTIME) { // don't write to FIFO
          spin_lock(&mbox_fifo_lock);
          continue;
        }
        // write to mbox_fifo
        *mbox_fifo_wr = mbox_data;
  
        spin_lock(&mbox_fifo_lock);
        // update write pointer
        mbox_fifo_wr++;
        // wrap around?
        if ( mbox_fifo_wr >= (mbox_fifo + 2*MBOX_FIFO_DEPTH) )
          mbox_fifo_wr = mbox_fifo;
        // full?
        if ( mbox_fifo_wr == mbox_fifo_rd )
          mbox_fifo_full = 1;
        if (DEBUG_LEVEL_MBOX > 0) {
          printk(KERN_INFO "PULP: Written %#x to mbox_fifo.\n",mbox_data);
          printk(KERN_INFO "PULP: mbox_fifo_wr: %d\n",(unsigned)(mbox_fifo_wr-mbox_fifo));
          printk(KERN_INFO "PULP: mbox_fifo_rd: %d\n",(unsigned)(mbox_fifo_rd-mbox_fifo));
          printk(KERN_INFO "PULP: mbox_fifo_full %d\n",mbox_fifo_full);
          if (DEBUG_LEVEL_MBOX > 1) {
            for (i=0; i<2*MBOX_FIFO_DEPTH; i++) {
              printk(KERN_INFO "mbox_fifo[%d]: %d %d %#x\n",i,
                     (&mbox_fifo[i] == mbox_fifo_wr) ? 1:0,
                     (&mbox_fifo[i] == mbox_fifo_rd) ? 1:0,
                     mbox_fifo[i]);
            }
          }
        }
        spin_unlock(&mbox_fifo_lock);
  
        read++;
      }
      spin_lock(&mbox_fifo_lock);
    } // while mailbox not empty and FIFO buffer not full
    spin_unlock(&mbox_fifo_lock);    
  
    // wake up user space process
    if ( read ) {
      wake_up_interruptible(&mbox_wq);
      if (DEBUG_LEVEL_MBOX > 0)
        printk(KERN_INFO "PULP: Read %d words from mailbox.\n",read);
    }
    // adjust receive interrupt threshold of mailbox interface
    else if ( mbox_fifo_full ) {
      printk(KERN_INFO "PULP: mbox_fifo_full %d\n",mbox_fifo_full);
    }
  }
  else if (mbox_is & 0x4) // mailbox error
    iowrite32(0x4,(void *)((unsigned long)mbox+MBOX_IS_OFFSET_B));
  else // mailbox send interrupt threshold - not used
    iowrite32(0x1,(void *)((unsigned long)mbox+MBOX_IS_OFFSET_B));

  if (DEBUG_LEVEL_MBOX > 0) {
    do_gettimeofday(&time);
    printk(KERN_INFO "PULP: Mailbox interrupt status: %#x. Interrupt handled at: %02li:%02li:%02li.\n",
           mbox_is,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
  }

  return;
}

/**
 * User-space applications wants to read data from mbox_fifo
 *
 * Standard Linux kernel read interface
 */
ssize_t pulp_mbox_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
  int i;
  unsigned mbox_data;
  unsigned long not_copied, flags;
  
  spin_lock_irqsave(&mbox_fifo_lock, flags);
  while ( (mbox_fifo_wr == mbox_fifo_rd) && !mbox_fifo_full ) { // nothing to read
    spin_unlock_irqrestore(&mbox_fifo_lock, flags); // release the spinlock
    
    if ( filp->f_flags & O_NONBLOCK )
      return -EAGAIN;
    if ( wait_event_interruptible(mbox_wq,
          (mbox_fifo_wr != mbox_fifo_rd) || mbox_fifo_full) )
      return -ERESTARTSYS; // signal: tell the fs layer to handle it
   
    spin_lock_irqsave(&mbox_fifo_lock, flags);
  }

  // mbox_fifo contains data to be read by user
  if ( mbox_fifo_wr > mbox_fifo_rd )
    count = min(count, (size_t)(mbox_fifo_wr - mbox_fifo_rd)*sizeof(mbox_fifo[0]));
  else // wrap, return data up to end of FIFO 
    count = min(count,
      (size_t)(mbox_fifo + 2*MBOX_FIFO_DEPTH - mbox_fifo_rd)*sizeof(mbox_fifo[0]));
 
  // release the spinlock, copy_to_user can sleep
  spin_unlock_irqrestore(&mbox_fifo_lock, flags); 

  not_copied = copy_to_user(buf, mbox_fifo_rd, count);
  
  spin_lock_irqsave(&mbox_fifo_lock, flags);
  // update read pointer
  mbox_fifo_rd = mbox_fifo_rd + (count - not_copied)/sizeof(mbox_fifo[0]);
  // wrap around
  if ( mbox_fifo_rd >= (mbox_fifo + 2*MBOX_FIFO_DEPTH) )
    mbox_fifo_rd = mbox_fifo;
  // not full anymore?
  if ( mbox_fifo_full && ((count - not_copied)/sizeof(mbox_fifo[0]) > 0) ) {
    mbox_fifo_full = 0;

    // if there is data available in the mailbox, read it to mbox_fifo (interrupt won't be triggered anymore)
    while ( !mbox_fifo_full &&
            !(0x1 & ioread32((void *)((unsigned long)pulp_mbox+MBOX_STATUS_OFFSET_B))) ) {

      // read mailbox
      mbox_data = ioread32((void *)((unsigned long)pulp_mbox+MBOX_RDDATA_OFFSET_B));
    
      if (mbox_data == RAB_UPDATE) {
        printk(KERN_WARNING "PULP: Read RAB_UPDATE request from mailbox in pulp_mbox_read. This request will be ignored.");
        continue;
      }
      else {
        if (mbox_data == TO_RUNTIME) // don't write to FIFO
          continue;
  
        // write to mbox_fifo
        *mbox_fifo_wr = mbox_data;

        // update write pointer
        mbox_fifo_wr++;
        // wrap around?
        if ( mbox_fifo_wr >= (mbox_fifo + 2*MBOX_FIFO_DEPTH) )
          mbox_fifo_wr = mbox_fifo;
        // full?
        if ( mbox_fifo_wr == mbox_fifo_rd )
          mbox_fifo_full = 1;
        if (DEBUG_LEVEL_MBOX > 0) {
          printk(KERN_INFO "PULP: Written %#x to mbox_fifo.\n",mbox_data);
          printk(KERN_INFO "PULP: mbox_fifo_wr: %d\n",(unsigned)(mbox_fifo_wr-mbox_fifo));
          printk(KERN_INFO "PULP: mbox_fifo_rd: %d\n",(unsigned)(mbox_fifo_rd-mbox_fifo));
          printk(KERN_INFO "PULP: mbox_fifo_full %d\n",mbox_fifo_full);
          if (DEBUG_LEVEL_MBOX > 1) {
            for (i=0; i<2*MBOX_FIFO_DEPTH; i++) {
              printk(KERN_INFO "mbox_fifo[%d]: %d %d %#x \n",i,
                     (&mbox_fifo[i] == mbox_fifo_wr) ? 1:0,
                     (&mbox_fifo[i] == mbox_fifo_rd) ? 1:0,
                     mbox_fifo[i]);
            }
          }
        }
      }
    }
  }
  if (DEBUG_LEVEL_MBOX > 0) {
    printk(KERN_INFO "PULP: Read from mbox_fifo.\n");
    printk(KERN_INFO "PULP: mbox_fifo_wr: %d\n",(unsigned)(mbox_fifo_wr-mbox_fifo));
    printk(KERN_INFO "PULP: mbox_fifo_rd: %d\n",(unsigned)(mbox_fifo_rd-mbox_fifo));
    printk(KERN_INFO "PULP: mbox_fifo_full %d\n",mbox_fifo_full);
  }
  spin_unlock_irqrestore(&mbox_fifo_lock, flags); // release the spinlock

  if (DEBUG_LEVEL_MBOX > 0) {
    printk(KERN_INFO "PULP: Read %li words from mbox_fifo.\n",
      (count - not_copied)/sizeof(mbox_fifo[0]));
  }

  if ( not_copied )
    return -EFAULT; // bad address
  else
    return count;
}
