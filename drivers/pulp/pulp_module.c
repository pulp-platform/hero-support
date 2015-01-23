#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* KERN_ALERT, container_of */
#include <linux/kdev_t.h>	/* major, minor */
#include <linux/interrupt.h>    /* interrupt handling */
#include <asm/io.h>		/* ioremap, iounmap, iowrite32 */
#include <linux/cdev.h>		/* cdev struct */
#include <linux/fs.h>		/* file_operations struct */
#include <linux/mm.h>           /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/pagemap.h>      /* page_cache_release() */
#include <asm/cacheflush.h>     /* __cpuc_flush_dcache_area, outer_cache.flush_range */
#include <asm/uaccess.h>	/* copy_to_user, copy_from_user, access_ok */
#include <linux/time.h>         /* do_gettimeofday */
#include <linux/ioctl.h>        /* ioctl */
#include <linux/slab.h>         /* kmalloc */

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
static int     pulp_open   (struct inode *inode, struct file *filp);
static int     pulp_release(struct inode *p_inode, struct file *filp);
static int     pulp_mmap   (struct file *filp, struct vm_area_struct *vma);
static long    pulp_ioctl  (struct file *filp, unsigned int cmd, unsigned long arg);

static irqreturn_t pulp_isr_eoc    (int irq, void *ptr);
static irqreturn_t pulp_isr_mailbox(int irq, void *ptr);
static irqreturn_t pulp_isr_rab    (int irq, void *ptr);

// important structs
struct file_operations pulp_fops = {
  .owner          = THIS_MODULE,
  .open           = pulp_open,
  .release        = pulp_release,
  .mmap           = pulp_mmap,
  .unlocked_ioctl = pulp_ioctl,
};

// static variables
static PulpDev my_dev;

static struct class *my_class; 
static struct timeval time;
static unsigned slcr_value;
static unsigned mailbox_is;
static char rab_interrupt_type[6];

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

  my_dev.mailbox = ioremap_nocache(MAILBOX_H_BASE_ADDR, MAILBOX_SIZE_B);
  printk(KERN_INFO "PULP: Mailbox mapped to virtual kernel space @ %#lx.\n",
	 (long unsigned int) my_dev.mailbox); 
 
  my_dev.slcr = ioremap_nocache(SLCR_BASE_ADDR,SLCR_SIZE_B);
  printk(KERN_INFO "PULP: Zynq SLCR mapped to virtual kernel space @ %#lx.\n",
	 (long unsigned int) my_dev.slcr); 

  my_dev.mpcore = ioremap_nocache(MPCORE_BASE_ADDR,MPCORE_SIZE_B);
  printk(KERN_INFO "PULP: Zynq MPCore register mapped to virtual kernel space @ %#lx.\n",
	 (long unsigned int) my_dev.mpcore); 

  my_dev.gpio = ioremap_nocache(H_GPIO_BASE_ADDR, H_GPIO_SIZE_B);
  printk(KERN_INFO "PULP: Host GPIO mapped to virtual kernel space @ %#lx.\n",
	 (long unsigned int) my_dev.gpio); 
  // remove GPIO reset, the driver and the runtime use the SLCR resets
  iowrite32(0x80000000,my_dev.gpio+0x8);

  // actually not needed - handled in user space
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
  mpcore_icdicfr3=ioread32(my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B);
  mpcore_icdicfr4=ioread32(my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B);
   
  // configure rising-edge active for 61, 63-65, and high-level active for 62
  mpcore_icdicfr3 &= 0x03FFFFFF; // delete bits 31 - 26
  mpcore_icdicfr3 |= 0xDC000000; // set bits 31 - 26: 11 01 11
    
  mpcore_icdicfr4 &= 0xFFFFFFF0; // delete bits 3 - 0
  mpcore_icdicfr4 |= 0x0000000F; // set bits 3 - 0: 11 11
     
  // write configuration
  iowrite32(mpcore_icdicfr3,my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B);
  iowrite32(mpcore_icdicfr4,my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B);
  
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

static void __exit pulp_exit(void) {
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

int pulp_open(struct inode *inode, struct file *filp) {
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

int pulp_release(struct inode *p_inode, struct file *filp) {

  printk(KERN_INFO "PULP: Device released.\n");
  return 0;
}

/***********************************************************************************
 *
 * mmap 
 *
 ***********************************************************************************/
int pulp_mmap(struct file *filp, struct vm_area_struct *vma) {

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
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B)) {
    // H_GPIO
    base_addr = H_GPIO_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B;
    size_b = H_GPIO_SIZE_B;
    strcpy(type,"Host GPIO");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B)) {
    // CLKING
    base_addr = CLKING_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B - H_GPIO_SIZE_B;
    size_b = CLKING_SIZE_B;
    strcpy(type,"Clking");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B)) {
    // STDOUT
    base_addr = STDOUT_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B - H_GPIO_SIZE_B - CLKING_SIZE_B;
    size_b = STDOUT_SIZE_B;
    strcpy(type,"Stdout");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B)) {
    // RAB config
    base_addr = RAB_CONFIG_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B - H_GPIO_SIZE_B - CLKING_SIZE_B - STDOUT_SIZE_B;
    size_b = RAB_CONFIG_SIZE_B;
    strcpy(type,"RAB config");
  }
  // Zynq System
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B)) {
    // Zynq SLCR
    base_addr = SLCR_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B - H_GPIO_SIZE_B - CLKING_SIZE_B - STDOUT_SIZE_B - RAB_CONFIG_SIZE_B;
    size_b = SLCR_SIZE_B;
    strcpy(type,"Zynq SLCR");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B + MPCORE_SIZE_B)) {
    // Zynq MPCore
    base_addr = MPCORE_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MAILBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B - H_GPIO_SIZE_B - CLKING_SIZE_B - STDOUT_SIZE_B - RAB_CONFIG_SIZE_B - SLCR_SIZE_B;
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

  printk(KERN_INFO "PULP: %s memory mapped. \nPhysical address = %#lx, user space virtual address = %#lx, vsize = %#lx.\n",type, physical << PAGE_SHIFT, vma->vm_start, vsize);

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
irqreturn_t pulp_isr_eoc(int irq, void *ptr) {
   
  // do something
  do_gettimeofday(&time);
  printk(KERN_INFO "PULP: End of Computation: %02li:%02li:%02li.\n",
	 (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
 
  // interrupt is just a pulse, no need to clear it
  return IRQ_HANDLED;
}
 
irqreturn_t pulp_isr_mailbox(int irq, void *ptr) {
  // do something
  do_gettimeofday(&time);
 
  // clear the interrupt
  mailbox_is = 0x7 & ioread32(my_dev.mailbox+MAILBOX_IS_OFFSET_B);
  iowrite32(0x7,my_dev.mailbox+MAILBOX_IS_OFFSET_B);
  printk(KERN_INFO "PULP: Mailbox Interface 0 interrupt status: %#x. Interrupt handled at: %02li:%02li:%02li.\n",
	 mailbox_is,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
 
  return IRQ_HANDLED;
}

irqreturn_t pulp_isr_rab(int irq, void *ptr) {
     
  do_gettimeofday(&time);
  
  // detect RAB interrupt type
  if (RAB_MISS_IRQ == irq)
    strcpy(rab_interrupt_type,"miss");
  else if (RAB_MULTI_IRQ == irq)
    strcpy(rab_interrupt_type,"multi");
  else // RAB_PROT_IRQ == irq
    strcpy(rab_interrupt_type,"prot");

  // interrupt is just a pulse, no need to clear it
  // instead reset PULP

  printk(KERN_INFO "PULP: RAB %s interrupt handled at %02li:%02li:%02li.\n",
	 rab_interrupt_type,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
  // read FPGA reset control register
  slcr_value = ioread32(my_dev.slcr+SLCR_FPGA_RST_CTRL_OFFSET_B);
  // extract the FPGA_OUT_RST bits
  slcr_value = slcr_value & 0xF;
  // // enable reset
  // iowrite32(slcr_value | (0x1 << SLCR_FPGA_OUT_RST),my_dev.slcr+SLCR_FPGA_RST_CTRL_OFFSET_B);
  // // wait
  // udelay(1000);
  // // disable reset
  // iowrite32(slcr_value & (0xF & (0x0 << SLCR_FPGA_OUT_RST)),my_dev.slcr+SLCR_FPGA_RST_CTRL_OFFSET_B);
  // printk(KERN_INFO "PULP: Device has been reset due to a RAB %s interrupt.\n",rab_interrupt_type);
 
  return IRQ_HANDLED;
}


/***********************************************************************************
 *
 * ioctl 
 *
 ***********************************************************************************/
long pulp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  int err = 0, i, j;
  long retval = 0;
  
  // to read from user space
  unsigned request[3];
  unsigned long ret, n_bytes_left;
  unsigned byte;

  // what we get from user space
  unsigned size_b;
   
  // what get_user_pages needs
  unsigned len; 
  struct page ** pages;
  
  // what mem_map_sg needs needs
  unsigned * addr_start_vec;
  unsigned * addr_end_vec;
  unsigned * addr_offset_vec;
  unsigned * page_idxs_start;
  unsigned * page_idxs_end;
  unsigned n_segments;

  // needed for cache flushing
  unsigned offset_start, offset_end;

  // needed for RAB managment
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;
 
  // needed for DMA management
  unsigned addr_l3, addr_pulp;
  unsigned char dma_cmd;
  unsigned addr_src, addr_dst;

  struct dma_async_tx_descriptor ** descs;

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
  if ( (_IOC_NR(cmd) < 0xB0) | (_IOC_NR(cmd) > 0xB3) ) return -ENOTTY;

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
      ret = __copy_from_user(request, (void __user *)arg, n_bytes_left);
      if (ret < 0) {
	printk(KERN_WARNING "PULP: cannot copy RAB config from user space.\n");
	return ret;
      }
      byte += ret;
      n_bytes_left -= ret;
    }
     
    // parse request
    RAB_GET_PROT(rab_slice_req->prot, request[0]);    
    RAB_GET_PORT(rab_slice_req->rab_port, request[0]);
    RAB_GET_DATE_EXP(rab_slice_req->date_exp, request[0]);
    RAB_GET_DATE_CUR(rab_slice_req->date_cur, request[0]);

    rab_slice_req->addr_start = request[1];
    size_b = request[2];
    
    rab_slice_req->addr_end = rab_slice_req->addr_start + size_b;
    n_segments = 1;

    if (DEBUG_LEVEL_RAB > 0) {
      printk(KERN_INFO "PULP: New RAB request:\n");
      printk(KERN_INFO "PULP: rab_port = %d.\n",rab_slice_req->rab_port);
      printk(KERN_INFO "PULP: date_exp = %d.\n",rab_slice_req->date_exp);
      printk(KERN_INFO "PULP: date_cur = %d.\n",rab_slice_req->date_cur);
    }
  
    // check type of remapping
    if ( (rab_slice_req->rab_port == 0) | (rab_slice_req->addr_start == L3_MEM_BASE_ADDR) ) 
      rab_slice_req->const_mapping = 1;
    else rab_slice_req->const_mapping = 0;
        
    if (rab_slice_req->const_mapping) { // constant remapping

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

    //  check for free field in page_ptrs list
    err = pulp_rab_page_ptrs_get_field(rab_slice_req);
    if (err) {
      return err;
    }

    /*
     *  setup the slices
     */ 
    // to do: protect with semaphore!?
    for (i=0;i<n_segments;i++) {
      
      if (!rab_slice_req->const_mapping) {
	rab_slice_req->addr_start = addr_start_vec[i];
	rab_slice_req->addr_end = addr_end_vec[i];
	rab_slice_req->addr_offset = addr_offset_vec[i];
	rab_slice_req->page_idx_start = page_idxs_start[i];
	rab_slice_req->page_idx_end = page_idxs_end[i];
      }

      // get a free slice
      err = pulp_rab_slice_get(rab_slice_req);
      if (err) {
	return err;
      }

      // free memory of slices to be re-configured
      pulp_rab_slice_free(rab_slice_req);
      
      // setup slice
      err = pulp_rab_slice_setup(my_dev.rab_config, rab_slice_req, pages);
      if (err) {
	return err;
      }

      // flush caches
      if (!rab_slice_req->const_mapping) {
	for (j=page_idxs_start[i]; j<page_idxs_start[i]+1; j++) {
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
    
    if (!rab_slice_req->const_mapping) {
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

    // check every slice on every port
    for (i=0;i<RAB_N_PORTS;i++) {
      rab_slice_req->rab_port = i;
      
      for (j=0;j<RAB_N_SLICES;j++) {
	rab_slice_req->rab_slice = j;
	
	if ( !rab_slice_req->date_cur ||
	     (rab_slice_req->date_cur == BIT_MASK_GEN(RAB_CONFIG_N_BITS_DATE)) ||
	     (pulp_rab_slice_check(rab_slice_req)) ) // free slice
	  pulp_rab_slice_free(rab_slice_req);
      }
    }
    break;

  case PULP_IOCTL_DMAC_XFER: // Setup a transfer using the PL330 DMAC inside Zynq
  
    // get transfer data from user space - arg already checked above
    ret = 1;
    byte = 0;
    n_bytes_left = 3*sizeof(unsigned); 
    while (ret > 0) {
      ret = __copy_from_user(request, (void __user *)arg, n_bytes_left);
      if (ret < 0) {
    	printk(KERN_WARNING "PULP: cannot copy DMAC transfer data from user space.\n");
    	return ret;
      }
      byte += ret;
      n_bytes_left -= ret;
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
    pulp_dma_cleanup[dma_cmd].descs = &descs;
    pulp_dma_cleanup[dma_cmd].pages = &pages;
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
      for (j=page_idxs_start[i]; j<(page_idxs_start[i]+1); j++) {
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

      // prepare the transfers, fill the descriptors
      err = pulp_dma_xfer_prep(&descs[i], &pulp_dma_chan[dma_cmd], addr_dst, addr_src, size_b, (i == n_segments-1));
      if (err) {
	printk(KERN_WARNING "PULP: Could not setup DMA transfer.\n");
	return err;
      }    
        
      // set callback parameters for last transaction
      if ( i == n_segments-1 ) {
      	descs[i]->callback = (dma_async_tx_callback)pulp_dma_xfer_cleanup;
      	descs[i]->callback_param = &pulp_dma_cleanup[dma_cmd];
      }

      // submit the transaction
      descs[i]->cookie = dmaengine_submit(descs[i]);
    }

#ifdef PROFILE_DMA
      time_dma_start = ktime_get();
#endif      

    // issue pending DMA requests and wait for callback notification
    dma_async_issue_pending(pulp_dma_chan[dma_cmd]);

#ifdef PROFILE_DMA 
    // wait for finish
    for (j=0;j<100000;j++) {
      ret = dma_async_is_tx_complete(pulp_dma_chan[dma_cmd],descs[n_segments-1]->cookie,NULL,NULL);
      if (!ret)
	break;
      udelay(10);
    }
// FIXME!!! -> kernel crashes very often
    //kfree(*descs); 

    // time measurement
    time_dma_end = ktime_get();
    time_dma_acc = ktime_us_delta(time_dma_end,time_dma_start);
    
    printk("PULP - DMA: size = %d [bytes]\n",request[2] & 0x7FFFFFFF);
    printk("PULP - DMA: time = %d [us]\n",time_dma_acc);
#endif
      
    break;

  default:
    return -ENOTTY;
  }

  return retval;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pirmin Vogel");
MODULE_DESCRIPTION("PULPonFPGA driver");
