#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* KERN_ALERT, container_of */
#include <linux/kdev_t.h> /* major, minor */
#include <linux/interrupt.h>    /* interrupt handling */
#include <linux/workqueue.h>    /* schedule non-atomic work */
#include <linux/spinlock.h>     /* locking in interrupt context*/
#include <linux/wait.h>         /* wait_queue */
#include <asm/io.h>   /* ioremap, iounmap, iowrite32 */
#include <linux/cdev.h>   /* cdev struct */
#include <linux/fs.h>   /* file_operations struct */
#include <linux/mm.h>           /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/pagemap.h>      /* page_cache_release() */
#include <asm/cacheflush.h>     /* __cpuc_flush_dcache_area, outer_cache.flush_range */
#include <asm/uaccess.h>  /* copy_to_user, copy_from_user, access_ok */
#include <linux/time.h>         /* do_gettimeofday */
#include <linux/ioctl.h>        /* ioctl */
#include <linux/slab.h>         /* kmalloc */
#include <linux/errno.h>        /* errno */

#include <linux/sched.h>
#include <linux/delay.h>        /* udelay */
#include <linux/device.h>       // class_create, device_create

/***************************************************************************************/
//#include <linux/wait.h>
#include <linux/ktime.h>      /* For ktime_get(), ktime_us_delta() */
/***************************************************************************************/

#include "zynq.h"
#include "pulp_host.h"

#include "pulp_module.h"
#include "pulp_mem.h"
#include "pulp_rab.h"
#include "pulp_dma.h"

// VM_RESERVERD for mmap
#ifndef VM_RESERVED
# define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

// methods declarations
static int     pulp_open        (struct inode *inode, struct file *filp);
static int     pulp_release     (struct inode *p_inode, struct file *filp);
static int     pulp_mmap        (struct file *filp, struct vm_area_struct *vma);
static long    pulp_ioctl       (struct file *filp, unsigned int cmd, unsigned long arg);
static ssize_t pulp_mailbox_read(struct file *filp, char __user *buf, size_t count, loff_t *offp);

static irqreturn_t pulp_isr_eoc    (int irq, void *ptr);
static irqreturn_t pulp_isr_mailbox(int irq, void *ptr);
static irqreturn_t pulp_isr_rab    (int irq, void *ptr);

static void pulp_rab_handle_miss(unsigned unused);

// important structs
struct file_operations pulp_fops = {
  .owner          = THIS_MODULE,
  .open           = pulp_open,
  .release        = pulp_release,
  .mmap           = pulp_mmap,
  .unlocked_ioctl = pulp_ioctl,
  .read           = pulp_mailbox_read,
};

// static variables
static PulpDev my_dev;

static struct class *my_class; 

// for ISR
static struct timeval time;
//static unsigned slcr_value;
static unsigned mailbox_data;
static unsigned mailbox_is;
static unsigned long flags;
static char rab_interrupt_type[6];

// for mailbox
static unsigned mailbox_fifo[MAILBOX_FIFO_DEPTH*2];
static unsigned mailbox_fifo_full = 0;
static unsigned * mailbox_fifo_rd = mailbox_fifo;
static unsigned * mailbox_fifo_wr = mailbox_fifo;
DEFINE_SPINLOCK(mailbox_fifo_lock);
DECLARE_WAIT_QUEUE_HEAD(mailbox_wq);

// for RAB
static RabStripeElem * elem_cur;
static RabStripeReq rab_stripe_req[RAB_N_MAPPINGS];
static unsigned rab_mapping;
static unsigned rab_mapping_active;
#if defined(PROFILE_RAB) || defined(PROFILE_RAB_MH)
static unsigned arm_clk_cntr_value = 0;
static unsigned arm_clk_cntr_value_start = 0;
static unsigned clk_cntr_response = 0;
static unsigned clk_cntr_update = 0;
static unsigned clk_cntr_setup = 0;
static unsigned clk_cntr_cleanup = 0;
static unsigned clk_cntr_cache_flush = 0;
static unsigned clk_cntr_get_user_pages = 0;
static unsigned clk_cntr_map_sg = 0;
static unsigned n_slices_updated = 0;
static unsigned n_pages_setup = 0;
static unsigned n_updates = 0;
static unsigned n_cleanups = 0;            
#endif

// for RAB miss handling
static char rab_mh_wq_name[10] = "RAB_MH_WQ";
static struct workqueue_struct *rab_mh_wq;
static struct work_struct rab_mh_w;
static struct task_struct *user_task;
static struct mm_struct *user_mm;
static unsigned rab_mh = 0;
static unsigned rab_mh_addr[RAB_MH_FIFO_DEPTH];
static unsigned rab_mh_id[RAB_MH_FIFO_DEPTH];
static unsigned rab_mh_date;
static unsigned rab_mh_use_acp = 0;
#ifdef PROFILE_RAB_MH
static unsigned clk_cntr_resp_tmp[N_CORES];
static unsigned clk_cntr_resp[N_CORES];
static unsigned clk_cntr_sched[N_CORES];
static unsigned clk_cntr_refill[N_CORES];
static unsigned n_misses = 0;
static unsigned n_first_misses = 0;
#endif

// for DMA
static struct dma_chan * pulp_dma_chan[2];
static DmaCleanup pulp_dma_cleanup[2];

// methods definitions
/***********************************************************************************
 *
 * init 
 *
 ***********************************************************************************/
static int __init pulp_init(void)
{
  int err;
  unsigned mpcore_icdicfr3, mpcore_icdicfr4;

  printk(KERN_ALERT "PULP: Loading device driver.\n");

  my_dev.minor = 0;
  
  /*
   *  init module char device
   */
  // get dynamic device numbers
  err = alloc_chrdev_region(&my_dev.dev, my_dev.minor, PULP_N_DEV_NUMBERS, "PULP");
  if (err) {
    printk(KERN_WARNING "PULP: Can't get major device number.\n");
    goto fail_alloc_chrdev_region;
  }
  // create class struct
  if ((my_class = class_create(THIS_MODULE, "pmca")) == NULL) {
    printk(KERN_WARNING "PULP: Error creating class.\n");
    err = -1;
    goto fail_create_class;
  }
  // create device and register it with sysfs
  if (device_create(my_class, NULL, my_dev.dev, NULL, "PULP") == NULL) {
    printk(KERN_WARNING "PULP: Error creating device.\n");
    err = -1;
    goto fail_create_device;
  }
  printk(KERN_INFO "PULP: Device created.\n");

  my_dev.major = MAJOR(my_dev.dev);
  my_dev.fops = &pulp_fops;

  /*
   *  struct cdev
   */
  cdev_init(&my_dev.cdev, my_dev.fops);
  my_dev.cdev.owner = THIS_MODULE;

  /**********
   *
   * ioremap
   *
   **********/
  my_dev.rab_config = ioremap_nocache(RAB_CONFIG_BASE_ADDR, RAB_CONFIG_SIZE_B);
  printk(KERN_INFO "PULP: RAB config mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.rab_config); 
  pulp_rab_init();
  rab_mapping_active = 0;

  my_dev.mailbox = ioremap_nocache(MAILBOX_H_BASE_ADDR, MAILBOX_SIZE_B);
  printk(KERN_INFO "PULP: Mailbox mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.mailbox); 
 
  my_dev.slcr = ioremap_nocache(SLCR_BASE_ADDR,SLCR_SIZE_B);
  printk(KERN_INFO "PULP: Zynq SLCR mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.slcr);
  // make sure to enable the PL clock
  if ( !BF_GET(ioread32((void *)((unsigned)my_dev.slcr+SLCR_FPGA0_THR_STA_OFFSET_B)),16,1) ) {
    iowrite32(0x0,(void *)((unsigned)my_dev.slcr+SLCR_FPGA0_THR_CNT_OFFSET_B));
    if ( !BF_GET(ioread32((void *)((unsigned)my_dev.slcr+SLCR_FPGA0_THR_STA_OFFSET_B)),16,1) ) {
      printk(KERN_WARNING "PULP: Could not enable reference clock for PULP.\n");
      goto fail_ioremap;
    }
  }
    
  my_dev.mpcore = ioremap_nocache(MPCORE_BASE_ADDR,MPCORE_SIZE_B);
  printk(KERN_INFO "PULP: Zynq MPCore register mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.mpcore); 

  my_dev.gpio = ioremap_nocache(H_GPIO_BASE_ADDR, H_GPIO_SIZE_B);
  printk(KERN_INFO "PULP: Host GPIO mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.gpio); 
  // remove GPIO reset, the driver and the runtime use the SLCR resets
  iowrite32(0xC0000000,(void *)((unsigned)my_dev.gpio+0x8));

  // PULP timer used for RAB performance monitoring
  my_dev.clusters = ioremap_nocache(CLUSTERS_H_BASE_ADDR, CLUSTERS_SIZE_B);
  printk(KERN_INFO "PULP: Clusters mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.clusters); 

  // actually not needed - handled in user space
  my_dev.soc_periph = ioremap_nocache(SOC_PERIPHERALS_H_BASE_ADDR, SOC_PERIPHERALS_SIZE_B);
  printk(KERN_INFO "PULP: SoC peripherals mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.soc_periph); 
  
  // actually not needed - handled in user space
  my_dev.l2_mem = ioremap_nocache(L2_MEM_H_BASE_ADDR, L2_MEM_SIZE_B);
  printk(KERN_INFO "PULP: L2 memory mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.l2_mem); 

  // actually not needed - handled in user space
  my_dev.l3_mem = ioremap_nocache(L3_MEM_H_BASE_ADDR, L3_MEM_SIZE_B);
  if (my_dev.l3_mem == NULL) {
    printk(KERN_WARNING "PULP: ioremap_nochache not allowed for non-reserved RAM.\n");
    err = EPERM;
    goto fail_ioremap;
  }
  printk(KERN_INFO "PULP: Shared L3 memory (DRAM) mapped to virtual kernel space @ %#lx.\n",
         (long unsigned int) my_dev.l3_mem);
 
  /*********************
   *
   * interrupts
   *
   *********************/
  // configure interrupt sensitivities: Zynq-7000 Technical Reference Manual, Section 7.2.4
  // read configuration
  mpcore_icdicfr3=ioread32((void *)((unsigned)my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B));
  mpcore_icdicfr4=ioread32((void *)((unsigned)my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B));
   
  // configure rising-edge active for 61-65
  mpcore_icdicfr3 &= 0x03FFFFFF; // delete bits 31 - 26
  mpcore_icdicfr3 |= 0xFC000000; // set bits 31 - 26: 11 11 11
    
  mpcore_icdicfr4 &= 0xFFFFFFF0; // delete bits 3 - 0
  mpcore_icdicfr4 |= 0x0000000F; // set bits 3 - 0: 11 11
     
  // write configuration
  iowrite32(mpcore_icdicfr3,(void *)((unsigned)my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B));
  iowrite32(mpcore_icdicfr4,(void *)((unsigned)my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B));
  
  // request interrupts and install handlers
  // eoc
  err = request_irq(END_OF_COMPUTATION_IRQ, pulp_isr_eoc, 0, "PULP", NULL);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
    goto fail_request_irq;
  }
  
  // mailbox
  err = request_irq(MAILBOX_IRQ, pulp_isr_mailbox, 0, "PULP", NULL);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
    goto fail_request_irq;
  }
    
  // RAB miss
  err = request_irq(RAB_MISS_IRQ, pulp_isr_rab, 0, "PULP", NULL);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
    goto fail_request_irq;
  }
  
  // RAB multi
  err = request_irq(RAB_MULTI_IRQ, pulp_isr_rab, 0, "PULP", NULL);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
    goto fail_request_irq;
  }
  
  // RAB prot
  err = request_irq(RAB_PROT_IRQ, pulp_isr_rab, 0, "PULP", NULL);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
    goto fail_request_irq;
  }
  /************************************
   *
   *  request DMA channels
   *
   ************************************/
  err = pulp_dma_chan_req(&pulp_dma_chan[0],0);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting DMA channel.\n");
    goto fail_request_dma;
  }
  err = pulp_dma_chan_req(&pulp_dma_chan[1],1);
  if (err) {
    printk(KERN_WARNING "PULP: Error requesting DMA channel.\n");
    goto fail_request_dma;
  }

  /************************************
   *
   *  tell the kernel about the device
   *
   ************************************/
  err = cdev_add(&my_dev.cdev, my_dev.dev, PULP_N_DEV_NUMBERS);
  if (err) {
    printk(KERN_WARNING "PULP: Error registering the device.\n");
    goto fail_register_device;
  }
  printk(KERN_INFO "PULP: Device registered.\n");

  return 0;

  /*
   * error handling
   */
 fail_register_device:
  pulp_dma_chan_clean(pulp_dma_chan[1]);
  pulp_dma_chan_clean(pulp_dma_chan[0]);
 fail_request_dma: 
  free_irq(END_OF_COMPUTATION_IRQ,NULL);
  free_irq(MAILBOX_IRQ,NULL);
  free_irq(RAB_MISS_IRQ,NULL);
  free_irq(RAB_MULTI_IRQ,NULL);
  free_irq(RAB_PROT_IRQ,NULL);
 fail_request_irq:
  iounmap(my_dev.rab_config); 
  iounmap(my_dev.mailbox);
  iounmap(my_dev.slcr);
  iounmap(my_dev.mpcore);
  iounmap(my_dev.gpio);
  iounmap(my_dev.clusters);
  iounmap(my_dev.soc_periph);
  iounmap(my_dev.l2_mem);
  iounmap(my_dev.l3_mem);
 fail_ioremap:
  cdev_del(&my_dev.cdev);
 fail_create_device: 
  class_destroy(my_class);
 fail_create_class: 
  unregister_chrdev_region(my_dev.dev, 1);
 fail_alloc_chrdev_region:
  return err;
}
module_init(pulp_init);

static void __exit pulp_exit(void)
{
  printk(KERN_ALERT "PULP: Unloading device driver.\n");
  // undo __init pulp_init
  pulp_dma_chan_clean(pulp_dma_chan[1]);
  pulp_dma_chan_clean(pulp_dma_chan[0]);
  free_irq(END_OF_COMPUTATION_IRQ,NULL);
  free_irq(MAILBOX_IRQ,NULL);
  free_irq(RAB_MISS_IRQ,NULL);
  free_irq(RAB_MULTI_IRQ,NULL);
  free_irq(RAB_PROT_IRQ,NULL);
  iounmap(my_dev.rab_config);
  iounmap(my_dev.mailbox);
  iounmap(my_dev.slcr);
  iounmap(my_dev.mpcore);
  iounmap(my_dev.gpio);
  iounmap(my_dev.clusters);
  iounmap(my_dev.soc_periph);
  iounmap(my_dev.l2_mem);
  iounmap(my_dev.l3_mem);
  cdev_del(&my_dev.cdev);
  device_destroy(my_class, my_dev.dev);
  class_destroy(my_class);
  unregister_chrdev_region(my_dev.dev, 1);
}
module_exit(pulp_exit);

int pulp_open(struct inode *inode, struct file *filp)
{
  // get a pointer to the PulpDev structure 
  PulpDev *dev;
  dev = container_of(inode->i_cdev, PulpDev, cdev);
  filp->private_data = dev;

  // // trim to 0 the length of the device if open is write_only
  // if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
  // 
  // }

  printk(KERN_INFO "PULP: Device opened.\n");
  
  return 0;
}

int pulp_release(struct inode *p_inode, struct file *filp)
{
  int i;
  
  // empty mailbox_fifo
  if ( (mailbox_fifo_wr != mailbox_fifo_rd) || mailbox_fifo_full) {
    for (i=0; i<2*MAILBOX_FIFO_DEPTH; i++) {
      printk(KERN_INFO "mailbox_fifo[%d]: %d %d %#x \n",i,
             (&mailbox_fifo[i] == mailbox_fifo_wr) ? 1:0,
             (&mailbox_fifo[i] == mailbox_fifo_rd) ? 1:0,
             mailbox_fifo[i]);
    }
  }
  mailbox_fifo_wr = mailbox_fifo;
  mailbox_fifo_rd = mailbox_fifo;

  printk(KERN_INFO "PULP: Device released.\n");
 
  return 0;
}

/***********************************************************************************
 *
 * mmap 
 *
 ***********************************************************************************/
int pulp_mmap(struct file *filp, struct vm_area_struct *vma)
{
  unsigned long base_addr; // base address to use for the remapping
  unsigned long size_b; // size for the remapping
  unsigned long off; // address offset in VMA
  unsigned long physical; // PFN of physical page
  unsigned long vsize;
  unsigned long psize;
  char type[12]; 
   
  off = (vma->vm_pgoff) << PAGE_SHIFT; 
  
  /*
   * based on the offset, set the proper base_addr and size_b, and adjust off  
   */
  // PULP internals
  if (off < CLUSTERS_SIZE_B) {
    // Clusters
    base_addr = CLUSTERS_H_BASE_ADDR;
    size_b = CLUSTERS_SIZE_B;
    strcpy(type,"Clusters");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B)) {
    // SoC peripherals
    base_addr = SOC_PERIPHERALS_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B;
    size_b = SOC_PERIPHERALS_SIZE_B;
    strcpy(type,"Peripherals");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B)) {
    // Mailbox
    base_addr = MAILBOX_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B;
    size_b = MAILBOX_SIZE_B;
    strcpy(type,"Mailbox");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B)) {
    // L2
    base_addr = L2_MEM_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B;
    size_b = L2_MEM_SIZE_B;
    strcpy(type,"L2");
  }
  // Platform
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B)) {
    // Shared L3
    base_addr = L3_MEM_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B;
    size_b = L3_MEM_SIZE_B;
    strcpy(type,"Shared L3");
  }
  // PULP external, PULPEmu 
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B)) {
    // H_GPIO
    base_addr = H_GPIO_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B;
    size_b = H_GPIO_SIZE_B;
    strcpy(type,"Host GPIO");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B)) {
    // CLKING
    base_addr = CLKING_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B;
    size_b = CLKING_SIZE_B;
    strcpy(type,"Clking");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B)) {
    // STDOUT
    base_addr = STDOUT_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B - CLKING_SIZE_B;
    size_b = STDOUT_SIZE_B;
    strcpy(type,"Stdout");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B)) {
    // RAB config
    base_addr = RAB_CONFIG_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B - CLKING_SIZE_B - STDOUT_SIZE_B;
    size_b = RAB_CONFIG_SIZE_B;
    strcpy(type,"RAB config");
  }
  // Zynq System
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B)) {
    // Zynq SLCR
    base_addr = SLCR_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B - CLKING_SIZE_B - STDOUT_SIZE_B - RAB_CONFIG_SIZE_B;
    size_b = SLCR_SIZE_B;
    strcpy(type,"Zynq SLCR");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B 
                  + MPCORE_SIZE_B)) {
    // Zynq MPCore
    base_addr = MPCORE_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B - CLKING_SIZE_B - STDOUT_SIZE_B - RAB_CONFIG_SIZE_B - SLCR_SIZE_B;
    size_b = MPCORE_SIZE_B;
    strcpy(type,"Zynq MPCore");
  }
  else {
    printk(KERN_INFO "PULP: Invalid VMA offset for mmap.\n");
    return -EINVAL;
  }

  // set physical PFN and virtual size
  physical = (base_addr + off) >> PAGE_SHIFT;
  vsize = vma->vm_end - vma->vm_start;
  psize = size_b - off;

  // set protection flags to avoid caching and paging
  vma->vm_flags |= VM_IO | VM_RESERVED;
  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

  printk(KERN_INFO
         "PULP: %s memory mapped. \nPhysical address = %#lx, user space virtual address = %#lx, vsize = %#lx.\n",
         type, physical << PAGE_SHIFT, vma->vm_start, vsize);

  if (vsize > psize)
    return -EINVAL; /*  spans too high */
  
  // map physical kernel space addresses to virtual user space addresses
  remap_pfn_range(vma, vma->vm_start, physical, vsize, vma->vm_page_prot);
  
  return 0;
}

// // avoid the extension of the mapping using mremap
// //   to avoid remapping past the end of the physical device area
// struct page *simple_nopage(struct vm_area_struct *vma, unsigned long address, int *type);
// {
//   return NOPAGE_SIGBUS;
// }

/***********************************************************************************
 *
 * interrupt service routines 
 *
 ***********************************************************************************/
irqreturn_t pulp_isr_eoc(int irq, void *ptr)
{  
  // do something
  do_gettimeofday(&time);
  printk(KERN_INFO "PULP: End of Computation: %02li:%02li:%02li.\n",
         (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
 
  // interrupt is just a pulse, no need to clear it
  return IRQ_HANDLED;
}
 
irqreturn_t pulp_isr_mailbox(int irq, void *ptr)
{
  int i,j;
  unsigned idx, n_slices, n_fields, offset, read, flags;

  if (DEBUG_LEVEL_MBOX > 0) {
    printk(KERN_INFO "PULP: Mailbox interrupt.\n");
  }

  // check interrupt status
  mailbox_is = 0x7 & ioread32((void *)((unsigned)my_dev.mailbox+MAILBOX_IS_OFFSET_B));
  
  if (mailbox_is & 0x2) { // mailbox receive threshold interrupt
    // clear the interrupt
    iowrite32(0x2,(void *)((unsigned)my_dev.mailbox+MAILBOX_IS_OFFSET_B));

    read = 0;

    spin_lock(&mailbox_fifo_lock);
    // while mailbox not empty and FIFO buffer not full
    while ( !(0x1 & ioread32((void *)((unsigned)my_dev.mailbox+MAILBOX_STATUS_OFFSET_B)))
            && !mailbox_fifo_full ) {
      spin_unlock(&mailbox_fifo_lock);

      // read mailbox
      mailbox_data = ioread32((void *)((unsigned)my_dev.mailbox+MAILBOX_RDDATA_OFFSET_B));

      if (mailbox_data == RAB_UPDATE) {
        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP: RAB update requested.\n");
        }

#ifdef PROFILE_RAB 
        // stop the PULP timer  
        iowrite32(0x1,(void *)((unsigned)my_dev.clusters+TIMER_STOP_OFFSET_B));
    
        // read the PULP timer
        clk_cntr_response += ioread32((void *)((unsigned)my_dev.clusters+TIMER_GET_TIME_LO_OFFSET_B));

        // reset the ARM clock counter
        asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
#endif

#if !defined(MEM_SHARING) || (MEM_SHARING == 2)
        rab_stripe_req[rab_mapping_active].stripe_idx++;
        idx = rab_stripe_req[rab_mapping_active].stripe_idx;
  
        // process every data element independently
        for (i=0; i<rab_stripe_req[rab_mapping_active].n_elements; i++) {
          elem_cur = rab_stripe_req[rab_mapping_active].elements[i];
          n_slices = (elem_cur->n_slices_per_stripe);
          n_fields = 1+elem_cur->n_slices;

          // process every slice independently
          for (j=0; j<n_slices; j++) {
            offset = 0x10*(elem_cur->rab_port*RAB_N_SLICES+elem_cur->slices[(idx & 0x1)*n_slices+j]);
  
            // deactivate slices
            iowrite32(0x0,(void *)((unsigned)my_dev.rab_config+offset+0x1c));
     
            // set up new translations rules
            if ( elem_cur->rab_stripes[idx*n_fields+j*2+1] ) { // offset != 0
              // set up the slice
              RAB_SLICE_SET_FLAGS(flags, elem_cur->prot, elem_cur->use_acp);
              iowrite32(elem_cur->rab_stripes[idx*n_fields+j*2],
                        (void *)((unsigned)my_dev.rab_config+offset+0x10)); // start_addr
              iowrite32(elem_cur->rab_stripes[idx*n_fields+j*2+2],
                        (void *)((unsigned)my_dev.rab_config+offset+0x14)); // end_addr
              iowrite32(elem_cur->rab_stripes[idx*n_fields+j*2+1],
                        (void *)((unsigned)my_dev.rab_config+offset+0x18)); // offset  
              // activate the slice
              iowrite32(flags, (void *)((unsigned)my_dev.rab_config+offset+0x1c));
#ifdef PROFILE_RAB 
              n_slices_updated++;
#endif
            }
          }
        }
#endif
        // signal ready to PULP
        iowrite32(HOST_READY,(void *)((unsigned)my_dev.mailbox+MAILBOX_WRDATA_OFFSET_B));
      
#ifdef PROFILE_RAB 
        // read the ARM clock counter
        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
        clk_cntr_update += arm_clk_cntr_value;

        n_updates++;

        // update counters in shared memory
        iowrite32(clk_cntr_response,
                  (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_RESPONSE_OFFSET_B));
        iowrite32(clk_cntr_update,
                  (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_UPDATE_OFFSET_B));
        iowrite32(n_slices_updated,
                  (void *)((unsigned)my_dev.l3_mem+N_SLICES_UPDATED_OFFSET_B));
        iowrite32(n_updates,
                  (void *)((unsigned)my_dev.l3_mem+N_UPDATES_OFFSET_B));
#endif    

#if defined(PROFILE_RAB) || defined(JPEG) 
        if (idx == rab_stripe_req[rab_mapping_active].n_stripes) {
          rab_stripe_req[rab_mapping_active].stripe_idx = 0;
          if (DEBUG_LEVEL_RAB > 0) {
            printk(KERN_INFO "PULP: RAB stripe table wrap around.\n");
          }
        }
#endif
        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP: RAB update completed.\n");
        }
      }
      else if (mailbox_data == RAB_SWITCH) {
        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP: RAB switch requested.\n");
        }

        // for debugging
        //pulp_rab_print_mapping(my_dev.rab_config, 0xAAAA);

        // switch RAB mapping
        rab_mapping_active = (rab_mapping_active) ? 0 : 1;
        pulp_rab_switch_mapping(my_dev.rab_config,rab_mapping_active);
        // reset striping idx
        rab_stripe_req[rab_mapping_active].stripe_idx = 0;

        if (DEBUG_LEVEL_RAB > 0) {
          printk(KERN_INFO "PULP: RAB switch completed.\n");
        }
          
        // for debugging
        //pulp_rab_print_mapping(my_dev.rab_config, 0xAAAA);

        // signal ready to PULP
        iowrite32(HOST_READY,(void *)((unsigned)my_dev.mailbox+MAILBOX_WRDATA_OFFSET_B));
      }
      else { // read to mailbox FIFO buffer
        if (mailbox_data == TO_RUNTIME) { // don't write to FIFO
          spin_lock(&mailbox_fifo_lock);
          continue;
        }
        // write to mailbox_fifo
        *mailbox_fifo_wr = mailbox_data;

        spin_lock(&mailbox_fifo_lock);
        // update write pointer
        mailbox_fifo_wr++;
        // wrap around?
        if ( mailbox_fifo_wr >= (mailbox_fifo + 2*MAILBOX_FIFO_DEPTH) )
          mailbox_fifo_wr = mailbox_fifo;
        // full?
        if ( mailbox_fifo_wr == mailbox_fifo_rd )
          mailbox_fifo_full = 1;
        if (DEBUG_LEVEL_MBOX > 0) {
          printk(KERN_INFO "PULP: Written %#x to mailbox_fifo.\n",mailbox_data);
          printk(KERN_INFO "PULP: mailbox_fifo_wr: %d\n",(unsigned)(mailbox_fifo_wr-mailbox_fifo));
          printk(KERN_INFO "PULP: mailbox_fifo_rd: %d\n",(unsigned)(mailbox_fifo_rd-mailbox_fifo));
          printk(KERN_INFO "PULP: mailbox_fifo_full %d\n",mailbox_fifo_full);
          if (DEBUG_LEVEL_MBOX > 1) {
            for (i=0; i<2*MAILBOX_FIFO_DEPTH; i++) {
              printk(KERN_INFO "mailbox_fifo[%d]: %d %d %#x\n",i,
                     (&mailbox_fifo[i] == mailbox_fifo_wr) ? 1:0,
                     (&mailbox_fifo[i] == mailbox_fifo_rd) ? 1:0,
                     mailbox_fifo[i]);
            }
          }
        }
        spin_unlock(&mailbox_fifo_lock);

        read++;
      }
      spin_lock(&mailbox_fifo_lock);
    } // while mailbox not empty and FIFO buffer not full
    spin_unlock(&mailbox_fifo_lock);    

    // wake up user space process
    if ( read ) {
      wake_up_interruptible(&mailbox_wq);
      if (DEBUG_LEVEL_MBOX > 0)
        printk(KERN_INFO "PULP: Read %d words from mailbox.\n",read);
    }
    // adjust receive interrupt threshold of mailbox interface
    else if ( mailbox_fifo_full ) {
      printk(KERN_INFO "PULP: mailbox_fifo_full %d\n",mailbox_fifo_full);
    }
  }
  else if (mailbox_is & 0x4) // mailbox error
    iowrite32(0x4,(void *)((unsigned)my_dev.mailbox+MAILBOX_IS_OFFSET_B));
  else // mailbox send interrupt threshold - not used
    iowrite32(0x1,(void *)((unsigned)my_dev.mailbox+MAILBOX_IS_OFFSET_B));

  if (DEBUG_LEVEL_MBOX > 0) {
    do_gettimeofday(&time);
    printk(KERN_INFO "PULP: Mailbox interrupt status: %#x. Interrupt handled at: %02li:%02li:%02li.\n",
           mailbox_is,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
  }

  return IRQ_HANDLED;
}

irqreturn_t pulp_isr_rab(int irq, void *ptr)
{    
  // detect RAB interrupt type
  if (RAB_MISS_IRQ == irq)
    strcpy(rab_interrupt_type,"miss");
  else if (RAB_MULTI_IRQ == irq)
    strcpy(rab_interrupt_type,"multi");
  else // RAB_PROT_IRQ == irq
    strcpy(rab_interrupt_type,"prot");

  if (RAB_MISS_IRQ == irq) {
    if (rab_mh) {

#ifdef PROFILE_RAB_MH
      // read PE timers
      int i;
      for (i=0; i<N_CORES; i++) {
        clk_cntr_resp_tmp[i] = ioread32((void *)((unsigned)my_dev.clusters+TIMER_GET_TIME_LO_OFFSET_B
                                                 +(i+1)*PE_TIMER_OFFSET_B));
      }
#endif
      // for debugging
      //pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);

      schedule_work(&rab_mh_w);
    }
  }

  if ( (DEBUG_LEVEL_RAB_MH > 1) || 
       ((RAB_MISS_IRQ == irq) && (0 == rab_mh)) ||
       ( RAB_MULTI_IRQ == irq || RAB_PROT_IRQ == irq ) ) {
    do_gettimeofday(&time);
    printk(KERN_INFO "PULP: RAB %s interrupt handled at %02li:%02li:%02li.\n",
           rab_interrupt_type,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);

    // for debugging
    pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);
  }

  return IRQ_HANDLED;
}


/***********************************************************************************
 *
 * ioctl 
 *
 ***********************************************************************************/
long pulp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  int err = 0, i, j, k;
  long retval = 0;
  
  // to read from user space
  unsigned request[3];
  unsigned long ret, n_bytes_left;
  unsigned byte;

  // what we get from user space
  unsigned size_b;
   
  // what get_user_pages needs
  struct page ** pages;
  unsigned len; 
  
  // what mem_map_sg needs needs
  unsigned * addr_start_vec;
  unsigned * addr_end_vec;
  unsigned * addr_offset_vec;
  unsigned * page_idxs_start;
  unsigned * page_idxs_end;
  unsigned n_segments;

  // needed for cache flushing
  unsigned offset_start, offset_end;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;

  // needed for RAB striping
  unsigned addr_start, addr_offset;
  unsigned n_entries, n_fields;
  unsigned char rab_port, prot, use_acp;
  unsigned seg_idx_start, seg_idx_end, n_slices;
  unsigned * max_stripe_size_b;
  unsigned ** rab_stripe_ptrs;
  unsigned shift, type;

  // needed for DMA management
  struct dma_async_tx_descriptor ** descs;
  unsigned addr_l3, addr_pulp;
  unsigned char dma_cmd;
  unsigned addr_src, addr_dst;

#ifdef PROFILE_DMA
  ktime_t time_dma_start, time_dma_end;
  unsigned time_dma_acc;
  time_dma_acc = 0;
#endif

  /*
   * extract the type and number bitfields, and don't decode wrong
   * cmds: return ENOTTY before access_ok()
   */
  if (_IOC_TYPE(cmd) != PULP_IOCTL_MAGIC) return -ENOTTY;
  if ( (_IOC_NR(cmd) < 0xB0) | (_IOC_NR(cmd) > 0xB6) ) return -ENOTTY;

  /*
   * the direction is a bitmask, and VERIFY_WRITE catches R/W
   * transfers. 'Type' is user-oriented, while access_ok is
   * kernel-oriented, so the concept of "read" and "write" is reversed
   */
  if (_IOC_DIR(cmd) & _IOC_READ)
    err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
  else if (_IOC_DIR(cmd) & _IOC_WRITE)
    err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
  if (err) return -EFAULT;

  // the actual ioctls
  switch(cmd) {

  case PULP_IOCTL_RAB_REQ: // Request new RAB slices

    // get slice data from user space - arg already checked above
    ret = 1;
    byte = 0;
    n_bytes_left = 3*sizeof(unsigned); 
    while (ret > 0) {
      ret = __copy_from_user((void *)((char *)request+byte),
                             (void __user *)((char *)arg+byte), n_bytes_left);
      if (ret < 0) {
        printk(KERN_WARNING "PULP: Cannot copy RAB config from user space.\n");
        return ret;
      }
      byte += (n_bytes_left - ret);
      n_bytes_left = ret;
    }
     
    // parse request
    RAB_GET_PROT(rab_slice_req->prot, request[0]);    
    RAB_GET_PORT(rab_slice_req->rab_port, request[0]);
    RAB_GET_USE_ACP(rab_slice_req->use_acp, request[0]);
    RAB_GET_OFFLOAD_ID(rab_slice_req->rab_mapping, request[0]);
    RAB_GET_DATE_EXP(rab_slice_req->date_exp, request[0]);
    RAB_GET_DATE_CUR(rab_slice_req->date_cur, request[0]);

    rab_slice_req->addr_start = request[1];
    size_b = request[2];
    
    rab_slice_req->addr_end = rab_slice_req->addr_start + size_b;
    n_segments = 1;

    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP: New RAB request:\n");
      printk(KERN_INFO "PULP: rab_port = %d.\n",rab_slice_req->rab_port);
      printk(KERN_INFO "PULP: use_acp = %d.\n",rab_slice_req->use_acp);
      printk(KERN_INFO "PULP: date_exp = %d.\n",rab_slice_req->date_exp);
      printk(KERN_INFO "PULP: date_cur = %d.\n",rab_slice_req->date_cur);
    }
  
    // check type of remapping
    if ( (rab_slice_req->rab_port == 0) || (rab_slice_req->addr_start == L3_MEM_BASE_ADDR) ) 
      rab_slice_req->flags = 0x1 | 0x4;
    //else rab_slice_req->flags = 0x0;
    // for now, set up every request on every mapping
    else
      rab_slice_req->flags = 0x4;
    
    if (rab_slice_req->flags & 0x1) { // constant remapping

      switch(rab_slice_req->addr_start) {
      
      case MAILBOX_H_BASE_ADDR:
        rab_slice_req->addr_offset = MAILBOX_BASE_ADDR - MAILBOX_SIZE_B; // Interface 0
        break;
    
      case L2_MEM_H_BASE_ADDR:
        rab_slice_req->addr_offset = L2_MEM_BASE_ADDR;
        break;

      case PULP_H_BASE_ADDR:
        rab_slice_req->addr_offset = PULP_BASE_ADDR;
        break;

      default: // L3_MEM_BASE_ADDR - port 1
        rab_slice_req->addr_offset = L3_MEM_H_BASE_ADDR;
      }

      len = 1;
    }
    else { // address translation required
            
      // number of pages
      len = pulp_mem_get_num_pages(rab_slice_req->addr_start,size_b);
      
      // get and lock user-space pages
      err = pulp_mem_get_user_pages(&pages, rab_slice_req->addr_start, len, rab_slice_req->prot & 0x4);
      if (err) {
        printk(KERN_WARNING "PULP: Locking of user-space pages failed.\n");
        return err;
      }
 
      // virtual to physcial address translation + segmentation
      n_segments = pulp_mem_map_sg(&addr_start_vec, &addr_end_vec, &addr_offset_vec,
                                   &page_idxs_start, &page_idxs_end, &pages, len, 
                                   rab_slice_req->addr_start, rab_slice_req->addr_end);
      if ( n_segments < 1 ) {
        printk(KERN_WARNING "PULP: Virtual to physical address translation failed.\n");
        return n_segments;
      }
    }

    /*
     *  setup the slices
     */ 
    // to do: protect with semaphore!?
    for (i=0;i<n_segments;i++) {
      
      if ( !(rab_slice_req->flags & 0x1) ) {
        rab_slice_req->addr_start = addr_start_vec[i];
        rab_slice_req->addr_end = addr_end_vec[i];
        rab_slice_req->addr_offset = addr_offset_vec[i];
        rab_slice_req->page_idx_start = page_idxs_start[i];
        rab_slice_req->page_idx_end = page_idxs_end[i];
      }

      // some requests need to be set up for every mapping
      for (j=0; j<RAB_N_MAPPINGS; j++) {

        if ( rab_slice_req->flags & 0x4 )
          rab_slice_req->rab_mapping = j;

        if ( rab_slice_req->rab_mapping == j ) {
  
          //  check for free field in page_ptrs list
          if ( !i ) {
            err = pulp_rab_page_ptrs_get_field(rab_slice_req);
            if (err) {
              return err;
            }
          }
    
          // get a free slice
          err = pulp_rab_slice_get(rab_slice_req);
          if (err) {
            return err;
          }
    
          // free memory of slices to be re-configured
          pulp_rab_slice_free(my_dev.rab_config, rab_slice_req);
      
          // setup slice
          err = pulp_rab_slice_setup(my_dev.rab_config, rab_slice_req, pages);
          if (err) {
            return err;
          }
        }
      }

      // flush caches
      if ( !(rab_slice_req->flags & 0x1) && !(rab_slice_req->use_acp) ) {
        for (j=page_idxs_start[i]; j<(page_idxs_end[i]+1); j++) {
          // flush the whole page?
          if (!i) 
            offset_start = BF_GET(addr_start_vec[i],0,PAGE_SHIFT);
          else
            offset_start = 0;

          if (i == (n_segments-1) )
            offset_end = BF_GET(addr_end_vec[i],0,PAGE_SHIFT);
          else 
            offset_end = PAGE_SIZE;

          pulp_mem_cache_flush(pages[j],offset_start,offset_end);
        }
      }
    }
    
    if ( !(rab_slice_req->flags & 0x1) ) {
      // kfree
      kfree(addr_start_vec);
      kfree(addr_end_vec);
      kfree(addr_offset_vec);
      kfree(page_idxs_start);
      kfree(page_idxs_end);
    }
    break;

  case PULP_IOCTL_RAB_FREE: // Free RAB slices based on time code

    // get current date    
    rab_slice_req->date_cur = BF_GET(arg,0,RAB_CONFIG_N_BITS_DATE);

    // unlock remapped pages
    //rab_slice_req->flags = 0x0;

    // check every slice on every port for every mapping
    for (i=0; i<RAB_N_MAPPINGS; i++) {
      rab_slice_req->rab_mapping = i;

      // unlock and release pages when freeing the last mapping
      if (i < (RAB_N_MAPPINGS-1) )
        rab_slice_req->flags = 0x4;
      else
        rab_slice_req->flags = 0x0;

      for (j=0;j<RAB_N_PORTS;j++) {
        rab_slice_req->rab_port = j;
      
        for (k=0;k<RAB_N_SLICES;k++) {
          rab_slice_req->rab_slice = k;
  
          if ( !rab_slice_req->date_cur ||
               (rab_slice_req->date_cur == RAB_MAX_DATE) ||
               (pulp_rab_slice_check(rab_slice_req)) ) // free slice
            pulp_rab_slice_free(my_dev.rab_config, rab_slice_req);
        }
      }
    }
    // reset rab_mapping_active
    rab_mapping_active = 0;

    // for debugging
    //pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);

    break;
    
  case PULP_IOCTL_RAB_REQ_STRIPED: // Request striped RAB slices
   
#ifdef PROFILE_RAB 
    // reset the ARM clock counter
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
#endif
 
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "New striped RAB request\n");
    }

    // get transfer data from user space - arg already checked above
    ret = 1;
    byte = 0;
    n_bytes_left = 3*sizeof(unsigned); 
    while (ret > 0) {
      ret = __copy_from_user((void *)((char *)request+byte),
                             (void __user *)((char *)arg+byte), n_bytes_left);
      if (ret < 0) {
        printk(KERN_WARNING "PULP: Cannot copy striped RAB config from user space.\n");
        return ret;
      }
      byte += (n_bytes_left - ret);
      n_bytes_left = ret;
    }

    // parse request
    RAB_GET_PROT(prot, request[0]);
    RAB_GET_PORT(rab_port, request[0]);
    RAB_GET_USE_ACP(use_acp, request[0]); // actually, this information could be retrieved on an element basis, similar to in/out/inout
    RAB_GET_OFFLOAD_ID(rab_mapping, request[0]);
    RAB_GET_N_ELEM(rab_stripe_req[rab_mapping].n_elements, request[0]);
    RAB_GET_N_STRIPES(rab_stripe_req[rab_mapping].n_stripes, request[0]);
   
    // allocate memory to hold the RabStripeElem structs
    rab_stripe_req[rab_mapping].elements = (RabStripeElem **)
      kmalloc((size_t)(rab_stripe_req[rab_mapping].n_elements*sizeof(RabStripeElem *)),GFP_KERNEL);

    max_stripe_size_b = (unsigned *)kmalloc((size_t)(rab_stripe_req[rab_mapping].n_elements
                                                     *sizeof(unsigned)),GFP_KERNEL);
    rab_stripe_ptrs = (unsigned **)kmalloc((size_t)(rab_stripe_req[rab_mapping].n_elements
                                                    *sizeof(unsigned *)),GFP_KERNEL);

    // get data from user space
    ret = 1;
    byte = 0;
    n_bytes_left = rab_stripe_req[rab_mapping].n_elements*sizeof(unsigned); 
    while (ret > 0) {
      ret = copy_from_user((void *)((char *)max_stripe_size_b+byte),
                           (void __user *)((char *)request[1]+byte), n_bytes_left);
      if (ret < 0) {
        printk(KERN_WARNING "PULP: Cannot copy stripe information from user space.\n");
        return ret;
      }
      byte += (n_bytes_left - ret);
      n_bytes_left = ret;
    }
    
    ret = 1;
    byte = 0;
    n_bytes_left = rab_stripe_req[rab_mapping].n_elements*sizeof(unsigned *); 
    while (ret > 0) {
      ret = copy_from_user((void *)((char *)rab_stripe_ptrs+byte),
                           (void __user *)((char *)request[2]+byte), n_bytes_left);
      if (ret < 0) {
        printk(KERN_WARNING "PULP: Cannot copy stripe information from user space.\n");
        return ret;
      }
      byte += (n_bytes_left - ret);
      n_bytes_left = ret;
    }
    
    if (DEBUG_LEVEL_RAB > 1) {
      printk(KERN_INFO "PULP: New striped RAB request\n");
      printk(KERN_INFO "PULP: n_elements = %d \n",rab_stripe_req[rab_mapping].n_elements);
      printk(KERN_INFO "PULP: n_stripes = %d \n",rab_stripe_req[rab_mapping].n_stripes);
    }

    // process every data element independently
    for (i=0; i<rab_stripe_req[rab_mapping].n_elements; i++) {
      
      // allocate memory to hold the RabStripeElem struct
      elem_cur = (RabStripeElem *)kmalloc((size_t)sizeof(RabStripeElem),GFP_KERNEL);
      
      // compute the maximum number of required slices (per stripe)
      n_slices = max_stripe_size_b[i]/PAGE_SIZE;
      if (n_slices*PAGE_SIZE < max_stripe_size_b[i])
        n_slices++; // remainder
      n_slices++;   // non-aligned

      if (DEBUG_LEVEL_RAB > 1) {
        printk(KERN_INFO "PULP: Element %d, n_slices = %d \n", i, n_slices);
      }

      // fill the RabStripeElem struct
      elem_cur->n_slices_per_stripe = n_slices;
      elem_cur->rab_port = rab_port;
      elem_cur->prot = prot;
      elem_cur->use_acp = use_acp;
      elem_cur->n_stripes = rab_stripe_req[rab_mapping].n_stripes;
      
      // allocate memory to hold rab_stripes
      n_fields = 2*n_slices + 1; // for every stripe: start, offset, start, offset, ..., end
      n_entries = n_fields*(elem_cur->n_stripes+1);
      elem_cur->rab_stripes = (unsigned *)kmalloc((size_t)(n_entries*sizeof(unsigned)),GFP_KERNEL);   
      
      // get data from user space
      ret = 1;
      byte = 0;
      n_bytes_left = n_entries*sizeof(unsigned);
      while (ret > 0) {
        ret = copy_from_user((void *)((char *)(elem_cur->rab_stripes)+byte),
                             (void __user *)((char *)rab_stripe_ptrs[i]+byte), n_bytes_left);
        if (ret < 0) {
          printk(KERN_WARNING "PULP: Cannot copy stripe information from user space.\n");
          return ret;
        }
        byte += (n_bytes_left - ret);
        n_bytes_left = ret;
      }
            
      // detect type of element: 2 = in, 3 = out, 4 = inout
      if ( elem_cur->rab_stripes[0] == 0 )
        type = 3;
      else if ( elem_cur->rab_stripes[n_entries-1] == 0 )
        type = 2;
      else
        type = 4;
      
      // allocate memory to hold slices
      if (type == 4)
        elem_cur->n_slices = 4*n_slices; // double buffering: *2 + inout: *2
      else
        elem_cur->n_slices = 2*n_slices; // double buffering: *2
      elem_cur->slices = (unsigned *)kmalloc((size_t)(elem_cur->n_slices*sizeof(unsigned)),GFP_KERNEL);

      if ( type == 3 ) {
        shift = 1;
        j = n_fields;
      }
      else {
        shift = 0;
        j = 0;
      }
      
      rab_slice_req->addr_start = elem_cur->rab_stripes[j];
      rab_slice_req->addr_end   = elem_cur->rab_stripes[elem_cur->n_stripes*n_fields+j-1];
      size_b = rab_slice_req->addr_end - rab_slice_req->addr_start;

      if (DEBUG_LEVEL_RAB > 2) {
        printk(KERN_INFO "PULP: Element %d: \n",i);
        printk(KERN_INFO "PULP: addr_start = %#x \n",rab_slice_req->addr_start);
        printk(KERN_INFO "PULP: addr_end   = %#x \n",rab_slice_req->addr_end);
        printk(KERN_INFO "PULP: size_b     = %#x \n",size_b);
      }

      // number of pages
      len = pulp_mem_get_num_pages(rab_slice_req->addr_start,size_b);

#ifdef PROFILE_RAB
      n_pages_setup += len;
#endif   

#ifdef PROFILE_RAB
      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
#endif   

      // get and lock user-space pages
      err = pulp_mem_get_user_pages(&pages, rab_slice_req->addr_start, len, rab_slice_req->prot & 0x4);
      if (err) {
        printk(KERN_WARNING "PULP: Locking of user-space pages failed.\n");
        return err;
      }

#ifdef PROFILE_RAB
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_get_user_pages += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif   
 
      // virtual to physcial address translation + segmentation
      n_segments = pulp_mem_map_sg(&addr_start_vec, &addr_end_vec, &addr_offset_vec,
                                   &page_idxs_start, &page_idxs_end, &pages, len, 
                                   rab_slice_req->addr_start, rab_slice_req->addr_end);
      if ( n_segments < 1 ) {
        printk(KERN_WARNING "PULP: Virtual to physical address translation failed.\n");
        return n_segments;
      }

#ifdef PROFILE_RAB
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_map_sg += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif   

      if(!elem_cur->use_acp) {
        // flush caches for all segments
        for (j=0; j<n_segments; j++) { 
          for (k=page_idxs_start[j]; k<(page_idxs_end[j]+1); k++) {
            // flush the whole page?
            if (!j) 
              offset_start = BF_GET(addr_start_vec[j],0,PAGE_SHIFT);
            else
              offset_start = 0;

            if (j == (n_segments-1) )
              offset_end = BF_GET(addr_end_vec[j],0,PAGE_SHIFT);
            else 
              offset_end = PAGE_SIZE;
  
            pulp_mem_cache_flush(pages[k],offset_start,offset_end);
          }
        }
      }

#ifdef PROFILE_RAB
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_cache_flush += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif   

      /*
       * fill rab_stripes table
       */
      seg_idx_start = 0;
      seg_idx_end = 0;
      for (j=0; j<(elem_cur->n_stripes+1); j++) {

        // detect dummy rows in rab_stripes table
        if ( (j == 0) && ( shift == 1) )
          continue;
        else if ( (j == elem_cur->n_stripes) && (shift == 0) )
          break;

        // determine the segment indices
        while ( ((seg_idx_start+1) < n_segments) && 
                (elem_cur->rab_stripes[j*n_fields] >= addr_start_vec[seg_idx_start+1]) ) {
          seg_idx_start++;
        }
        while ( (seg_idx_end < n_segments) && 
                (elem_cur->rab_stripes[j*n_fields+n_fields-1] > addr_end_vec[seg_idx_end]) ) {
          seg_idx_end++;
        }
  
        n_slices = seg_idx_end - seg_idx_start + 1; // number of required slices
        if ( n_slices > (elem_cur->n_slices_per_stripe) ) {
          printk(KERN_WARNING "PULP: Stripe %d of Element %d touches too many memory segments.\n",j,i);
          //printk(KERN_INFO "%d slices reserved, %d segments\n",elem_cur->n_slices_per_stripe, n_segments);
          //printk(KERN_INFO "start segment = %d, end segment = %d\n",seg_idx_start,seg_idx_end);
        }

        if (DEBUG_LEVEL_RAB > 3) {
          printk(KERN_INFO "PULP: Stripe %d: seg_idx_start = %d, seg_idx_end = %d \n",
                 j,seg_idx_start,seg_idx_end);
        }

        // extract the physical addresses + virtual start addresses of the segments 
        for (k=0; k<(elem_cur->n_slices_per_stripe); k++) {
          if (k == 0) {
            addr_start  = elem_cur->rab_stripes[j*n_fields];
            addr_offset = addr_offset_vec[seg_idx_start];
            // inter-page offset: for multi-page segments
            addr_offset += (BF_GET(addr_start,PAGE_SHIFT,32-PAGE_SHIFT) << PAGE_SHIFT)
              - (BF_GET(addr_start_vec[seg_idx_start],PAGE_SHIFT,32-PAGE_SHIFT) << PAGE_SHIFT); 
            // intra-page offset: no additional offset for first slice of first stripe
            if ( !(j == 0) && !((j == 1) && (shift == 1)) )
              addr_offset += (elem_cur->rab_stripes[j*n_fields] & BIT_MASK_GEN(PAGE_SHIFT));
          }
          else if ( k < n_slices ) {
            addr_start  = addr_start_vec[seg_idx_start + k];
            addr_offset = addr_offset_vec[seg_idx_start + k];
          }
          else if ( k == n_slices ) { // put addr_end as addr_start for first unused slice
            addr_start = elem_cur->rab_stripes[j*n_fields+n_fields-1]; 
            addr_offset = 0;
          }
          else { // not all slices used for that stripe
            addr_start = 0;
            addr_offset = 0;
          }
          // write data to table
          elem_cur->rab_stripes[j*n_fields+k*2]   = addr_start;
          elem_cur->rab_stripes[j*n_fields+k*2+1] = addr_offset;
        }
      }
      if (DEBUG_LEVEL_RAB > 3) {
        for (j=0; j<(elem_cur->n_stripes+1) ;j++) {
          printk(KERN_INFO "PULP: Stripe %d:\n",j);
          for (k=0; k<n_fields; k++) {
            printk(KERN_INFO "%#x\n",elem_cur->rab_stripes[j*n_fields+k]);
          }
          printk(KERN_INFO "\n");
        }
      } 

      /*
       *  request the slices
       */ 
      // to do: protect with semaphore!?
      rab_slice_req->rab_mapping = rab_mapping;
      rab_slice_req->rab_port = elem_cur->rab_port;
      rab_slice_req->use_acp = elem_cur->use_acp;

      // do not overwrite any remappings, the remappings will be freed manually
      rab_slice_req->date_cur = 0x1;
      rab_slice_req->date_exp = RAB_MAX_DATE; // also avoid check in pulp_rab_slice_setup

      // assign all pages to all slices
      rab_slice_req->page_idx_start = page_idxs_start[0];
      rab_slice_req->page_idx_end = page_idxs_end[n_segments-1];

      //  check for free field in page_ptrs list
      err = pulp_rab_page_ptrs_get_field(rab_slice_req);
      if (err) {
        return err;
      }

      for (j=0; j<elem_cur->n_slices; j++) {
        // set up only the used slices of the first stripe, the others need just to be requested  
        if ( (j>(elem_cur->n_slices_per_stripe-1)) || (elem_cur->rab_stripes[j*2+1] == 0) ) {
          rab_slice_req->addr_start  = 0;
          rab_slice_req->addr_end    = 0;
          rab_slice_req->addr_offset = 0;
          rab_slice_req->prot        = prot & 0x6;
        }
        else {
          rab_slice_req->addr_start  = elem_cur->rab_stripes[j*2];
          rab_slice_req->addr_end    = elem_cur->rab_stripes[j*2+2];
          rab_slice_req->addr_offset = elem_cur->rab_stripes[j*2+1];
          rab_slice_req->prot        = prot;
        }
        // force check in pulp_rab_slice_setup
        if (j == 0)
          rab_slice_req->flags = 0x0; 
        else
          rab_slice_req->flags = 0x2; // striped mode, not constant
  
        // get a free slice
        err = pulp_rab_slice_get(rab_slice_req);
        if (err) {
          return err;
        }
        elem_cur->slices[j] = rab_slice_req->rab_slice;

        // free memory of slices to be re-configured
        pulp_rab_slice_free(my_dev.rab_config, rab_slice_req);
            
        // set up slice
        err = pulp_rab_slice_setup(my_dev.rab_config, rab_slice_req, pages);
        if (err) {
          return err;
        }
      }

      if (DEBUG_LEVEL_RAB > 2) {
        printk(KERN_INFO "PULP: Element %d: \n",i);
        printk(KERN_INFO "PULP: Striped slices: \n");
        for (j=0; j<elem_cur->n_slices ;j++) {
          printk(KERN_INFO "%d, ",elem_cur->slices[j]);
        }
        printk(KERN_INFO "\n");
      } 

      // store the pointer to the RabStripeElem struct
      rab_stripe_req[rab_mapping].elements[i] = elem_cur;

      // free memory
      kfree(addr_start_vec);
      kfree(addr_end_vec);
      kfree(addr_offset_vec);
      kfree(page_idxs_start);
      kfree(page_idxs_end);
    }

    rab_stripe_req[rab_mapping].stripe_idx = 0;

#ifdef PROFILE_RAB
    // read the ARM clock counter
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
    clk_cntr_setup += arm_clk_cntr_value;

    // update counters in shared memory
    iowrite32(clk_cntr_setup,
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_SETUP_OFFSET_B));
    iowrite32(clk_cntr_cache_flush,
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_CACHE_FLUSH_OFFSET_B));
    iowrite32(clk_cntr_get_user_pages,
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_GET_USER_PAGES_OFFSET_B));
    iowrite32(clk_cntr_map_sg,
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_MAP_SG_OFFSET_B));
    iowrite32(n_pages_setup,
              (void *)((unsigned)my_dev.l3_mem+N_PAGES_SETUP_OFFSET_B));
#endif

    // for debugging
    //pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);

    break;

  case PULP_IOCTL_RAB_FREE_STRIPED: // Free striped RAB slices

#ifdef PROFILE_RAB 
    // reset the ARM clock counter
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
#endif

    // get offload ID/RAB mapping
    rab_mapping = BF_GET(arg,0,RAB_CONFIG_N_BITS_OFFLOAD_ID);
    rab_slice_req->rab_mapping = rab_mapping;

    if ( rab_stripe_req[rab_mapping].n_elements > 0 ) {

      // process every data element independently
      for (i=0; i<rab_stripe_req[rab_mapping].n_elements; i++) {

        if (DEBUG_LEVEL_RAB > 1) {
          printk(KERN_INFO "Shared Element %d:\n",i);
        }
      
        elem_cur = rab_stripe_req[rab_mapping].elements[i];
     
        // free the slices
        for (j=0; j<elem_cur->n_slices; j++){
          rab_slice_req->rab_port = elem_cur->rab_port;
          rab_slice_req->rab_slice = elem_cur->slices[j];

          // unlock and release pages when freeing the last slice
          if (j < (elem_cur->n_slices-1) )
            rab_slice_req->flags = 0x2;
          else
            rab_slice_req->flags = 0x0;

          pulp_rab_slice_free(my_dev.rab_config, rab_slice_req);
        }

        // free memory
        kfree(elem_cur->slices);
        kfree(elem_cur->rab_stripes);
        kfree(elem_cur);
      }

      //free memory
      kfree(rab_stripe_req[rab_mapping].elements);
    
      // mark the stripe request as freed
      rab_stripe_req[rab_mapping].n_elements = 0;
    }

#ifdef PROFILE_RAB 
    // read the ARM clock counter
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
    clk_cntr_cleanup += arm_clk_cntr_value;

    n_cleanups++;

    // update counters in shared memory
    iowrite32(clk_cntr_cleanup,(void *)((unsigned)my_dev.l3_mem+CLK_CNTR_CLEANUP_OFFSET_B));
    iowrite32(n_cleanups,(void *)((unsigned)my_dev.l3_mem+N_CLEANUPS_OFFSET_B));

    // cleanup driver variables
    clk_cntr_response = 0;   
    clk_cntr_update = 0;     
    clk_cntr_setup = 0;      
    clk_cntr_cleanup = 0;
    clk_cntr_cache_flush = 0;     
    clk_cntr_get_user_pages = 0;      
    clk_cntr_map_sg = 0;    
    n_slices_updated = 0;    
    n_pages_setup = 0;       
    n_updates = 0;           
    n_cleanups = 0;          
#endif
       
    break;

  case PULP_IOCTL_DMAC_XFER: // Setup a transfer using the PL330 DMAC inside Zynq
  
    // get transfer data from user space - arg already checked above
    ret = 1;
    byte = 0;
    n_bytes_left = 3*sizeof(unsigned);
    while (ret > 0) {
      ret = __copy_from_user((void *)((char *)request+byte),
                             (void __user *)((char *)arg+byte), n_bytes_left);
      if (ret < 0) {
        printk(KERN_WARNING "PULP: Cannot copy DMAC transfer data from user space.\n");
        return ret;
      }
      byte += (n_bytes_left - ret);
      n_bytes_left = ret;
    }
    
    addr_l3   = request[0];
    addr_pulp = request[1];
    
    size_b = request[2] & 0x7FFFFFFF;
    dma_cmd = (request[2] >> 31); 
    
    if (DEBUG_LEVEL_DMA > 0) {
      printk(KERN_INFO "PULP: New DMA request:\n");
      printk(KERN_INFO "PULP: addr_l3   = %#x.\n",addr_l3);
      printk(KERN_INFO "PULP: addr_pulp = %#x.\n",addr_pulp);
      printk(KERN_INFO "PULP: size_b = %#x.\n",size_b);
      printk(KERN_INFO "PULP: dma_cmd = %#x.\n",dma_cmd);
    }
    
    // number of pages
    len = pulp_mem_get_num_pages(addr_l3, size_b);
    
    // get and lock user-space pages
    ret = pulp_mem_get_user_pages(&pages, addr_l3, len, dma_cmd);
    if (err) {
      printk(KERN_WARNING "PULP: Locking of user-space pages failed.\n");
      return err;
    }    
   
    // virtual to physcial address translation + segmentation
    n_segments = pulp_mem_map_sg(&addr_start_vec, &addr_end_vec, &addr_offset_vec,
                                 &page_idxs_start, &page_idxs_end, &pages, len, 
                                 addr_l3, addr_l3+size_b);
    if ( n_segments < 1 ) {
      printk(KERN_WARNING "PULP: Virtual to physical address translation failed.\n");
      return n_segments;
    }
    
    // allocate memory to hold the transaction descriptors
    descs = (struct dma_async_tx_descriptor **)
      kmalloc((size_t)(n_segments*sizeof(struct dma_async_tx_descriptor *)),GFP_KERNEL);

    // prepare cleanup
    pulp_dma_cleanup[dma_cmd].descs = descs;
    pulp_dma_cleanup[dma_cmd].pages = pages;
    pulp_dma_cleanup[dma_cmd].n_pages = len;

    /*
     *  setup the transfers
     */ 
    size_b = 0;
    for (i=0;i<n_segments;i++) {
  
      addr_pulp += size_b;

      if (dma_cmd) { // PULP -> L3 // not yet tested
        addr_src = addr_pulp;
        addr_dst = addr_offset_vec[i];
      }
      else { // L3 -> PULP
        addr_src = addr_offset_vec[i]; 
        addr_dst = addr_pulp;
      }
      size_b = addr_end_vec[i] - addr_start_vec[i];

      // flush caches
      for (j=page_idxs_start[i]; j<(page_idxs_end[i]+1); j++) {
        // flush the whole page?
        if (!i) 
          offset_start = BF_GET(addr_start_vec[i],0,PAGE_SHIFT);
        else
          offset_start = 0;
  
        if ( i == (n_segments-1) )
          offset_end = BF_GET(addr_end_vec[i],0,PAGE_SHIFT);
        else 
          offset_end = PAGE_SIZE;
  
        pulp_mem_cache_flush(pages[j],offset_start,offset_end);
      }

      // prepare the transfers, fill the descriptors
      err = pulp_dma_xfer_prep(&descs[i], &pulp_dma_chan[dma_cmd],
                               addr_dst, addr_src, size_b, (i == n_segments-1));
      if (err) {
        printk(KERN_WARNING "PULP: Could not setup DMA transfer.\n");
        return err;
      }    
  
      // set callback parameters for last transaction
      if ( i == (n_segments-1) ) {
        descs[i]->callback = (dma_async_tx_callback)pulp_dma_xfer_cleanup;
        descs[i]->callback_param = &pulp_dma_cleanup[dma_cmd];
      }

      // submit the transaction
      descs[i]->cookie = dmaengine_submit(descs[i]);
    }
    if (DEBUG_LEVEL_DMA > 1)
      printk(KERN_INFO "PULP: DMA transactions for %i segments submitted.\n",n_segments);

#ifdef PROFILE_DMA
    time_dma_start = ktime_get();
#endif      
    // issue pending DMA requests and wait for callback notification
    dma_async_issue_pending(pulp_dma_chan[dma_cmd]);

    if (DEBUG_LEVEL_DMA > 1)
      printk(KERN_INFO "PULP: Pending DMA transactions issued.\n");

#ifdef PROFILE_DMA 
    // wait for finish
    for (j=0;j<100000;j++) {
      ret = dma_async_is_tx_complete(pulp_dma_chan[dma_cmd],descs[n_segments-1]->cookie,NULL,NULL);
      if (!ret)
        break;
      udelay(10);
    }
    // free transaction descriptors array
    kfree(descs); 

    // time measurement
    time_dma_end = ktime_get();
    time_dma_acc = ktime_us_delta(time_dma_end,time_dma_start);
    
    printk("PULP - DMA: size = %d [bytes]\n",request[2] & 0x7FFFFFFF);
    printk("PULP - DMA: time = %d [us]\n",time_dma_acc);
#endif
      
    break;

  case PULP_IOCTL_RAB_MH_ENA:

    // get slice data from user space - arg already checked above
    ret = 1;
    byte = 0;
    n_bytes_left = sizeof(unsigned); 
    while (ret > 0) {
      ret = __copy_from_user((void *)((char *)rab_mh_use_acp+byte),
                             (void __user *)((char *)arg+byte), n_bytes_left);
      if (ret < 0) {
        printk(KERN_WARNING "PULP: Cannot copy ACP flag from user space.\n");
        return ret;
      }
      byte += (n_bytes_left - ret);
      n_bytes_left = ret;
    }

    // create workqueue for RAB miss handling
    //rab_mh_wq = alloc_workqueue("%s", WQ_UNBOUND | WQ_HIGHPRI, 0, rab_mh_wq_name);
    rab_mh_wq = alloc_workqueue("%s", WQ_UNBOUND | WQ_HIGHPRI, 1, rab_mh_wq_name); // ST workqueue for strict ordering
    if (rab_mh_wq == NULL) {
      printk(KERN_WARNING "PULP: Allocation of workqueue for RAB miss handling failed.\n");
      return -ENOMEM;
    }

    // initialize the workqueue
    INIT_WORK(&rab_mh_w, (void *)pulp_rab_handle_miss);
    
    // register information of the calling user-space process
    user_task = current;
    user_mm = current->mm;

#ifdef PROFILE_RAB_MH
    for (i=0; i<N_CORES; i++) {
      clk_cntr_refill[i] = 0;
      clk_cntr_sched[i] = 0;
      clk_cntr_resp[i] = 0;
    }
    n_misses = 0;
    n_first_misses = 0;
    clk_cntr_setup = 0;
    clk_cntr_cache_flush = 0;
    clk_cntr_get_user_pages = 0;
    iowrite32(0, (void *)((unsigned)my_dev.l3_mem+N_MISSES_OFFSET_B));
#endif

    // enable - protect with semaphore?
    rab_mh = 1;
    rab_mh_date = 1;

    printk(KERN_INFO "PULP: RAB miss handling enabled.\n");

    break;

  case PULP_IOCTL_RAB_MH_DIS:
   
    // disable - protect with semaphore?
    rab_mh = 0;
    rab_mh_date = 0;

#ifdef PROFILE_RAB_MH
    for (i=0; i<N_CORES; i++) {
      printk(KERN_INFO "clk_cntr_refill[%d] = %d \n",i,clk_cntr_refill[i]);
      printk(KERN_INFO "clk_cntr_sched[%d]  = %d \n",i,clk_cntr_sched[i]);
      printk(KERN_INFO "clk_cntr_resp[%d]   = %d \n",i,clk_cntr_resp[i]);
    }
    printk(KERN_INFO "n_misses       = %d \n",n_misses);
    printk(KERN_INFO "n_first_misses = %d \n",n_first_misses);
    printk(KERN_INFO "clk_cntr_setup          = %d \n",ARM_PMU_CLK_DIV*clk_cntr_setup);
    printk(KERN_INFO "clk_cntr_cache_flush    = %d \n",ARM_PMU_CLK_DIV*clk_cntr_cache_flush);
    printk(KERN_INFO "clk_cntr_get_user_pages = %d \n",ARM_PMU_CLK_DIV*clk_cntr_get_user_pages);
#endif

    // flush and destroy the workqueue
    destroy_workqueue(rab_mh_wq);

    printk(KERN_INFO "PULP: RAB miss handling disabled.\n");

    break;

  default:
    return -ENOTTY;
  }

  return retval;
}

/***********************************************************************************
 *
 * read 
 *
 ***********************************************************************************/
ssize_t pulp_mailbox_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
  int i;
  unsigned long not_copied;
  
  spin_lock_irqsave(&mailbox_fifo_lock, flags);
  while ( (mailbox_fifo_wr == mailbox_fifo_rd) && !mailbox_fifo_full ) { // nothing to read
    spin_unlock_irqrestore(&mailbox_fifo_lock, flags); // release the spinlock
    
    if ( filp->f_flags & O_NONBLOCK )
      return -EAGAIN;
    if ( wait_event_interruptible(mailbox_wq, (mailbox_fifo_wr != mailbox_fifo_rd) || mailbox_fifo_full) )
      return -ERESTARTSYS; // signal: tell the fs layer to handle it
   
    spin_lock_irqsave(&mailbox_fifo_lock, flags);
  }

  // mailbox_fifo contains data to be read by user
  if ( mailbox_fifo_wr > mailbox_fifo_rd )
    count = min(count, (size_t)(mailbox_fifo_wr - mailbox_fifo_rd)*sizeof(mailbox_fifo[0]));
  else // wrap, return data up to end of FIFO 
    count = min(count, (size_t)(mailbox_fifo + 2*MAILBOX_FIFO_DEPTH - mailbox_fifo_rd)*sizeof(mailbox_fifo[0]));
 
  // release the spinlock, copy_to_user can sleep
  spin_unlock_irqrestore(&mailbox_fifo_lock, flags); 

  not_copied = copy_to_user(buf, mailbox_fifo_rd, count);
  
  spin_lock_irqsave(&mailbox_fifo_lock, flags);
  // update read pointer
  mailbox_fifo_rd = mailbox_fifo_rd + (count - not_copied)/sizeof(mailbox_fifo[0]);
  // wrap around
  if ( mailbox_fifo_rd >= (mailbox_fifo + 2*MAILBOX_FIFO_DEPTH) )
    mailbox_fifo_rd = mailbox_fifo;
  // not full anymore?
  if ( mailbox_fifo_full && ((count - not_copied)/sizeof(mailbox_fifo[0]) > 0) ) {
    mailbox_fifo_full = 0;

    // if there is data available in the mailbox, read it to mailbox_fifo (interrupt won't be triggered anymore)
    while ( !mailbox_fifo_full &&
            !(0x1 & ioread32((void *)((unsigned)my_dev.mailbox+MAILBOX_STATUS_OFFSET_B))) ) {

      // read mailbox
      mailbox_data = ioread32((void *)((unsigned)my_dev.mailbox+MAILBOX_RDDATA_OFFSET_B));
    
      if (mailbox_data == RAB_UPDATE) {
        printk(KERN_WARNING "PULP: Read RAB_UPDATE request from mailbox in pulp_mailbox_read. This request will be ignored.");
        continue;
      }
      else {
        if (mailbox_data == TO_RUNTIME) // don't write to FIFO
          continue;
  
        // write to mailbox_fifo
        *mailbox_fifo_wr = mailbox_data;

        // update write pointer
        mailbox_fifo_wr++;
        // wrap around?
        if ( mailbox_fifo_wr >= (mailbox_fifo + 2*MAILBOX_FIFO_DEPTH) )
          mailbox_fifo_wr = mailbox_fifo;
        // full?
        if ( mailbox_fifo_wr == mailbox_fifo_rd )
          mailbox_fifo_full = 1;
        if (DEBUG_LEVEL_MBOX > 0) {
          printk(KERN_INFO "PULP: Written %#x to mailbox_fifo.\n",mailbox_data);
          printk(KERN_INFO "PULP: mailbox_fifo_wr: %d\n",(unsigned)(mailbox_fifo_wr-mailbox_fifo));
          printk(KERN_INFO "PULP: mailbox_fifo_rd: %d\n",(unsigned)(mailbox_fifo_rd-mailbox_fifo));
          printk(KERN_INFO "PULP: mailbox_fifo_full %d\n",mailbox_fifo_full);
          if (DEBUG_LEVEL_MBOX > 1) {
            for (i=0; i<2*MAILBOX_FIFO_DEPTH; i++) {
              printk(KERN_INFO "mailbox_fifo[%d]: %d %d %#x \n",i,
                     (&mailbox_fifo[i] == mailbox_fifo_wr) ? 1:0,
                     (&mailbox_fifo[i] == mailbox_fifo_rd) ? 1:0,
                     mailbox_fifo[i]);
            }
          }
        }
      }
    }
  }
  if (DEBUG_LEVEL_MBOX > 0) {
    printk(KERN_INFO "PULP: Read from mailbox_fifo.\n");
    printk(KERN_INFO "PULP: mailbox_fifo_wr: %d\n",(unsigned)(mailbox_fifo_wr-mailbox_fifo));
    printk(KERN_INFO "PULP: mailbox_fifo_rd: %d\n",(unsigned)(mailbox_fifo_rd-mailbox_fifo));
    printk(KERN_INFO "PULP: mailbox_fifo_full %d\n",mailbox_fifo_full);
  }
  spin_unlock_irqrestore(&mailbox_fifo_lock, flags); // release the spinlock

  if (DEBUG_LEVEL_MBOX > 0) {
    printk(KERN_INFO "PULP: Read %li words from mailbox_fifo.\n",(count - not_copied)/sizeof(mailbox_fifo[0]));
  }

  if ( not_copied )
    return -EFAULT; // bad address
  else
    return count;
}

/***********************************************************************************
 *
 * RAB miss handling routine 
 *
 ***********************************************************************************/
static void pulp_rab_handle_miss(unsigned unused)
{
  int err, i, j, handled, result;
  unsigned id, id_pe, id_cluster;
  unsigned addr_start, addr_phys;
    
  // what get_user_pages needs
  unsigned long start;
  struct page **pages;
  unsigned write;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;

  if (DEBUG_LEVEL_RAB_MH > 1)
    printk(KERN_INFO "PULP: RAB miss handling routine started.\n");

  // empty miss-handling FIFOs
  for (i=0; i<RAB_MH_FIFO_DEPTH; i++) {
    rab_mh_addr[i] = ioread32((void *)((unsigned)my_dev.rab_config+RAB_MH_ADDR_FIFO_OFFSET_B));
    rab_mh_id[i]   = ioread32((void *)((unsigned)my_dev.rab_config+RAB_MH_ID_FIFO_OFFSET_B));
  
    // detect empty FIFOs
    if ( (rab_mh_addr[i] & 0x1) || (rab_mh_id[i] & 0x80000000) )
      break;
    if (DEBUG_LEVEL_RAB_MH > 0) {
      printk(KERN_INFO "PULP: RAB miss - i = %d, date = %#x, id = %#x, addr = %#x\n",
             i,rab_mh_date, rab_mh_id[i], rab_mh_addr[i]);
    }
     
    // handle every miss separately
    err = 0;
    addr_start = rab_mh_addr[i];
    id = rab_mh_id[i];
    
    // check the ID
    id_pe = BF_GET(id, 0, AXI_ID_WIDTH_CORE);
    //id_cluster = BF_GET(id, AXI_ID_WIDTH_CLUSTER + AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_SOC) - 0x3;
    id_cluster = BF_GET(id, AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_CLUSTER) - 0x2;

    // identify RAB port - for now, only handle misses on Port 1: PULP -> Host
    if ( BF_GET(id, AXI_ID_WIDTH, 1) )
      rab_slice_req->rab_port = 1;    
    else {
      printk(KERN_WARNING "PULP: Cannot handle RAB miss on ports different from Port 1.\n");
      printk(KERN_WARNING "PULP: RAB miss ID %#x.\n",id);
      continue;
    }

    // only handle misses from PEs' data interfaces
    if ( ( BF_GET(id, AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_CLUSTER) != 0x2 ) ||
         ( (id_cluster < 0) || (id_cluster > (N_CLUSTERS-1)) ) || 
         ( (id_pe < 0) || (id_pe > N_CORES-1) ) ) {
      printk(KERN_WARNING "PULP: Can only handle RAB misses originating from PE's data interfaces. id = %#x | addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);

      // for debugging
      pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);

      continue;
    }
  
#ifdef PROFILE_RAB_MH
    err = ioread32((void *)((unsigned)my_dev.l3_mem+N_MISSES_OFFSET_B));
    // reset internal counters
    if (!err) {      
      clk_cntr_setup = 0;
      clk_cntr_cache_flush = 0;
      clk_cntr_get_user_pages = 0;
      n_first_misses = 0;
      n_misses = 0;
      for (j=0; j<N_CORES; j++) {
        clk_cntr_refill[j] = 0;
        clk_cntr_sched[j]  = 0;
        clk_cntr_resp[j]   = 0;
      }
    }
    n_misses++;

    // read the PE timer
    clk_cntr_sched[id_pe] += ioread32((void *)((unsigned)my_dev.clusters
                                               +TIMER_GET_TIME_LO_OFFSET_B+(id_pe+1)*PE_TIMER_OFFSET_B));
    // save resp_tmp
    clk_cntr_resp[id_pe] += clk_cntr_resp_tmp[id_pe];
    
#endif

    // check if there has been a miss to the same page before
    handled = 0;    
    for (j=0; j<i; j++) {
      if ( addr_start == rab_mh_addr[j] ) {
        handled = 1;
        if (DEBUG_LEVEL_RAB_MH > 0) {
          printk(KERN_WARNING "PULP: Already handled a miss to this page.\n");
          // for debugging only - deactivate fetch enable
          // iowrite32(0xC0000000,(void *)((unsigned)my_dev.gpio+0x8));
        }
        break;
      }
    }

    // handle a miss
    if (!handled) {
  
#ifdef PROFILE_RAB_MH
      // reset the ARM clock counter
      asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );

      n_first_misses++;
#endif

      if (DEBUG_LEVEL_RAB_MH > 1) {
        printk(KERN_INFO "PULP: Trying to handle RAB miss to user-space virtual address %#x.\n",addr_start);
      }

      // what get_user_pages returns
      pages = (struct page **)kmalloc((size_t)(sizeof(struct page *)),GFP_KERNEL);
      if (pages == NULL) {
        printk(KERN_WARNING "PULP: Memory allocation failed.\n");
        err = 1;
        goto miss_handling_error;
      }

      // align address to page border / 4kB
      start = (unsigned long)(addr_start & BF_MASK_GEN(PAGE_SHIFT,32-PAGE_SHIFT));
      
      // get pointer to user-space buffer and lock it into memory, get a single page
      // try read/write mode first - fall back to read only
      write = 1;
      down_read(&user_task->mm->mmap_sem);
      result = get_user_pages(user_task, user_task->mm, start, 1, write, 0, pages, NULL);
      if ( result == -EFAULT ) {
        write = 0;
        result = get_user_pages(user_task, user_task->mm, start, 1, write, 0, pages, NULL);
      }
      up_read(&user_task->mm->mmap_sem);
      //current->mm = user_task->mm;
      //result = get_user_pages_fast(start, 1, 1, pages);
      if (result != 1) {
        printk(KERN_WARNING "PULP: Could not get requested user-space virtual address %#x.\n", addr_start);
        err = 1;
        goto miss_handling_error;
      }
      
      // virtual-to-physical address translation
      addr_phys = (unsigned)page_to_phys(pages[0]);
      if (DEBUG_LEVEL_MEM > 1) {
        printk(KERN_INFO "PULP: Physical address = %#x\n",addr_phys);
      }

#ifdef PROFILE_RAB_MH
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_get_user_pages += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif
      
      // fill rab_slice_req structure
      rab_slice_req->flags = 0;
      rab_slice_req->prot = (write << 2) | 0x2 | 0x1;
      rab_slice_req->use_acp = rab_mh_use_acp;

      // allocate a fixed number of slices to each PE???
      // works for one port only!!!
      rab_slice_req->date_exp = (unsigned char)rab_mh_date; 
      rab_slice_req->date_cur = (unsigned char)rab_mh_date;

      rab_slice_req->addr_start  = (unsigned)start;
      rab_slice_req->addr_end    = (unsigned)start + PAGE_SIZE;
      rab_slice_req->addr_offset = addr_phys;
      rab_slice_req->page_idx_start = 0;
      rab_slice_req->page_idx_end   = 0;

      rab_slice_req->rab_mapping = rab_mapping_active;

      // check for free field in page_ptrs list
      err = pulp_rab_page_ptrs_get_field(rab_slice_req);
      if (err) {
        goto miss_handling_error;
      }

      // get a free slice
      err = pulp_rab_slice_get(rab_slice_req);
      if (err) {
        goto miss_handling_error;
      }
 
      // free memory of slices to be re-configured
      pulp_rab_slice_free(my_dev.rab_config, rab_slice_req);

      // setup slice
      err = pulp_rab_slice_setup(my_dev.rab_config, rab_slice_req, pages);
      if (err) {
        goto miss_handling_error;
      }

#ifdef PROFILE_RAB_MH
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_setup += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif

      // flush the entire page from the caches if ACP is not used
      if (!rab_mh_use_acp)
        pulp_mem_cache_flush(pages[0],0,PAGE_SIZE);

      if (rab_slice_req->rab_slice == (RAB_N_SLICES - 1)) {
        rab_mh_date++;
        if (rab_mh_date > RAB_MAX_DATE_MH)
          rab_mh_date = 0;
        //printk(KERN_INFO "rab_mh_date = %d\n",rab_mh_date);
      }

#ifdef PROFILE_RAB_MH
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_cache_flush += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif
    }
    
    // // for debugging
    // pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);
    // msleep(1000);

    // wake up the sleeping PE
    iowrite32(BIT_N_SET(id_pe),
              (void *)((unsigned)my_dev.clusters + CLUSTER_SIZE_B*id_cluster
                       + CLUSTER_PERIPHERALS_OFFSET_B + BBMUX_CLKGATE_OFFSET_B + GP_2_OFFSET_B));

    /*
     * printk(KERN_INFO "WAKEUP: value = %#x, address = %#x\n",BIT_N_SET(id_pe), CLUSTER_SIZE_B*id_cluster \
     *        + CLUSTER_PERIPHERALS_OFFSET_B + BBMUX_CLKGATE_OFFSET_B + GP_2_OFFSET_B );
     */

#ifdef PROFILE_RAB_MH
    // read the PE timer
    clk_cntr_refill[id_pe] += ioread32((void *)((unsigned)my_dev.clusters
                                                +TIMER_GET_TIME_LO_OFFSET_B+(id_pe+1)*PE_TIMER_OFFSET_B));
    // update counters in shared memory
    iowrite32(n_misses, (void *)((unsigned)my_dev.l3_mem+N_MISSES_OFFSET_B));
#endif
    
    // error handling
  miss_handling_error:
    if (err) {
      printk(KERN_WARNING "PULP: Cannot handle RAB miss: id = %#x, addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);

      // for debugging
      pulp_rab_print_mapping(my_dev.rab_config,0xAAAA);
    }

  }
  if (DEBUG_LEVEL_RAB_MH > 1)
    printk(KERN_INFO "PULP: RAB miss handling routine finished.\n");
 
#ifdef PROFILE_RAB_MH
  // update counters in shared memory
  iowrite32(n_first_misses,
            (void *)((unsigned)my_dev.l3_mem+N_FIRST_MISSES_OFFSET_B));
  iowrite32(clk_cntr_setup,
            (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_SETUP_OFFSET_B));
  iowrite32(clk_cntr_cache_flush,
            (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_CACHE_FLUSH_OFFSET_B));
  iowrite32(clk_cntr_get_user_pages,
            (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_GET_USER_PAGES_OFFSET_B));

  for (j=0; j<N_CORES; j++) {
    iowrite32(clk_cntr_refill[j],
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_REFILL_OFFSET_B+4*j));
    iowrite32(clk_cntr_sched[j],
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_SCHED_OFFSET_B+4*j));
    iowrite32(clk_cntr_resp[j],
              (void *)((unsigned)my_dev.l3_mem+CLK_CNTR_RESP_OFFSET_B+4*j));
  }

#endif

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pirmin Vogel");
MODULE_DESCRIPTION("PULPonFPGA driver");
