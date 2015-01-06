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
//#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>        /* udelay */
#include <linux/device.h>       // class_create, device_create

#include "../../lib/inc/zynq.h"
#include "../../lib/inc/pulp_host.h"

#define RAB_DEBUG_LEVEL 2

// VM_RESERVERD for mmap
#ifndef VM_RESERVED
# define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

// methods declarations
int pulp_open(struct inode *inode, struct file *filp);
int pulp_release(struct inode *p_inode, struct file *filp);
ssize_t pulp_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
int pulp_mmap(struct file *filp, struct vm_area_struct *vma);
long pulp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

irqreturn_t pulp_isr_eoc(int irq, void *ptr);
irqreturn_t pulp_isr_mailbox(int irq, void *ptr);
irqreturn_t pulp_isr_rab(int irq, void *ptr);

// important structs
struct file_operations pulp_fops = {
  .owner = THIS_MODULE,
  //.llseek = pulp_llseek,
  .open = pulp_open,
  .release = pulp_release,
  //.read = pulp_read,
  //.write = pulp_write,
  .mmap = pulp_mmap,
  .unlocked_ioctl = pulp_ioctl,
};

// type definitions -> move to .h ???
typedef struct {
  dev_t dev; // device number
  struct file_operations *fops;
  struct cdev cdev;
  int minor;
  int major;
  void *l3_mem;
  void *l2_mem;
  void *mailbox;
  void *soc_periph;
  void *clusters;
  void *rab_config;
  void *gpio;
  void *slcr;
  void *mpcore;
} PulpDev;

// global variables
PulpDev my_dev;

// static variables
static struct class *my_class; 
static struct timeval time;
static unsigned slcr_value;
static unsigned mailbox_is;
static char rab_interrupt_type[6];
#define RAB_TABLE_WIDTH 3
static unsigned rab_slices[RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES];
static struct page ** page_ptrs[RAB_N_PORTS*RAB_N_SLICES];
static unsigned page_ptr_ref_cntrs[RAB_N_PORTS*RAB_N_SLICES];

// methods definitions
/***********************************************************************************
 *
 * init 
 *
 ***********************************************************************************/
static int __init pulp_init(void) {
  
  int err, i;
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
  printk(KERN_INFO "PULP: RAB config mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.rab_config); 
  // initialize the RAB slices table
  for (i=0;i<RAB_TABLE_WIDTH*RAB_N_PORTS*RAB_N_SLICES;i++)
    rab_slices[i] = 0;
  // initialize the RAB pages and reference counter lists
  for (i=0;i<RAB_N_PORTS*RAB_N_SLICES;i++) {
    page_ptrs[i] = 0;
    page_ptr_ref_cntrs[i] = 0;
  }

  my_dev.mailbox = ioremap_nocache(MAILBOX_H_BASE_ADDR, MAILBOX_SIZE_B);
  printk(KERN_INFO "PULP: Mailbox mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.mailbox); 
 
  my_dev.slcr = ioremap_nocache(SLCR_BASE_ADDR,SLCR_SIZE_B);
  printk(KERN_INFO "PULP: Zynq SLCR mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.slcr); 

  my_dev.mpcore = ioremap_nocache(MPCORE_BASE_ADDR,MPCORE_SIZE_B);
  printk(KERN_INFO "PULP: Zynq MPCore register mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.mpcore); 

  my_dev.gpio = ioremap_nocache(H_GPIO_BASE_ADDR, H_GPIO_SIZE_B);
  printk(KERN_INFO "PULP: Host GPIO mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.gpio); 
  // remove GPIO reset, the driver and the runtime use the SLCR resets
  iowrite32(0x80000000,my_dev.gpio+0x8);

  // actually not needed - handled in user space
  my_dev.clusters = ioremap_nocache(CLUSTERS_H_BASE_ADDR, CLUSTERS_SIZE_B);
  printk(KERN_INFO "PULP: Clusters mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.clusters); 

  // actually not needed - handled in user space
  my_dev.soc_periph = ioremap_nocache(SOC_PERIPHERALS_H_BASE_ADDR, SOC_PERIPHERALS_SIZE_B);
  printk(KERN_INFO "PULP: SoC peripherals mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.soc_periph); 
  
  // actually not needed - handled in user space
  my_dev.l2_mem = ioremap_nocache(L2_MEM_H_BASE_ADDR, L2_MEM_SIZE_B);
  printk(KERN_INFO "PULP: L2 memory mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.l2_mem); 

  // actually not needed - handled in user space
  my_dev.l3_mem = ioremap_nocache(L3_MEM_H_BASE_ADDR, L3_MEM_SIZE_B);
  if (my_dev.l3_mem == NULL) {
    printk(KERN_WARNING "PULP: ioremap_nochache not allowed for non-reserved RAM.\n");
    err = EPERM;
    goto fail_ioremap;
  }
  printk(KERN_INFO "PULP: Shared L3 memory (DRAM) mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.l3_mem);
 
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
  printk(KERN_INFO "PULP: End of Computation: %02li:%02li:%02li.\n",(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
 
  // interrupt is just a pulse, no need to clear it
 
  return IRQ_HANDLED;
}
 
irqreturn_t pulp_isr_mailbox(int irq, void *ptr) {
  // do something
  do_gettimeofday(&time);
 
  // clear the interrupt
  mailbox_is = 0x7 & ioread32(my_dev.mailbox+MAILBOX_IS_OFFSET_B);
  iowrite32(0x7,my_dev.mailbox+MAILBOX_IS_OFFSET_B);
  printk(KERN_INFO "PULP: Mailbox Interface 0 interrupt status: %#x. Interrupt handled at: %02li:%02li:%02li.\n",mailbox_is,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
 
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

  printk(KERN_INFO "PULP: RAB %s interrupt handled at %02li:%02li:%02li.\n",rab_interrupt_type,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
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
  int err = 0, i, j, k;
  long retval = 0;
  
  // to read from user space
  unsigned request[3];
  unsigned long ret, n_bytes_left;
  unsigned byte, const_mapping;

  // what we get from user space
  unsigned addr_start, size_b;
  unsigned char rab_port, prot, date_exp, date_cur;
  
  // what get_user_pages needs
  unsigned long start;
  unsigned len; 
  int result;
  struct page ** pages;
  unsigned * addr_phys;

  // what the RAB needs
  unsigned addr_end, addr_offset;
  unsigned * addr_start_vec;
  unsigned * addr_end_vec;
  unsigned * addr_offset_vec;
  unsigned rab_slice, offset, n_slices;

  // what is needed to manage the RAB table (rab_slices)
  unsigned * page_start_idxs;
  unsigned * page_end_idxs;
  unsigned page_idx_start_old, page_idx_end_old;
  unsigned page_ptr_idx, page_ptr_idx_old;
  struct page ** pages_old;
 
  /*
   * extract the type and number bitfields, and don't decode wrong
   * cmds: return ENOTTY before access_ok()
   */
  if (_IOC_TYPE(cmd) != PULP_IOCTL_MAGIC) return -ENOTTY;
  if ( (_IOC_NR(cmd) < 0xB0) | (_IOC_NR(cmd) > 0xB2) ) return -ENOTTY;

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
    prot = request[0] & 0x7;    
    rab_port = BF_GET(request[0], RAB_CONFIG_N_BITS_PROT, RAB_CONFIG_N_BITS_PORT);
    date_exp = BF_GET(request[0], RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, RAB_CONFIG_N_BITS_DATE);
    date_cur = BF_GET(request[0], RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT + RAB_CONFIG_N_BITS_DATE, RAB_CONFIG_N_BITS_DATE);
    addr_start = request[1];
    size_b = request[2];
    
    addr_end = addr_start + size_b;
    n_slices = 1;

    if (RAB_DEBUG_LEVEL > 0) {
      printk(KERN_INFO "rab_port %d.\n",rab_port);
      printk(KERN_INFO "date_exp %d.\n",date_exp);
      printk(KERN_INFO "date_cur %d.\n",date_cur);
    }
    
    // check type of remapping
    if ( (rab_port == 0) | (addr_start == L3_MEM_BASE_ADDR) ) const_mapping = 1;
    else const_mapping = 0;
    
    
    if (const_mapping) { // constant remapping

      switch(addr_start) {
      
      case MAILBOX_H_BASE_ADDR:
	addr_offset = MAILBOX_BASE_ADDR - MAILBOX_SIZE_B; // Interface 0
	break;
		
      case L2_MEM_H_BASE_ADDR:
	addr_offset = L2_MEM_BASE_ADDR;
	break;

   case PULP_H_BASE_ADDR:
	addr_offset = PULP_BASE_ADDR;
	break;

      default: // L3_MEM_BASE_ADDR - port 1
	addr_offset = L3_MEM_H_BASE_ADDR;
      }

      len = 1;
    }
    else { // address translation required
            
      // align to page size / 4kB
      start = (unsigned long)(addr_start >> PAGE_SHIFT);
      // number of pages
      len = size_b >> PAGE_SHIFT;
      if (BF_GET(size_b,0,PAGE_SHIFT) | BF_GET(addr_start,0,PAGE_SHIFT)) len++; // less than one page or not aligned to page border
      if ((addr_start >> PAGE_SHIFT) != ((addr_start + size_b) >> PAGE_SHIFT)) len++; // touches two pages 
      printk(KERN_INFO "len = %d\n",len);

      // what get_user_pages returns
      pages = (struct page **)kmalloc((size_t)(len*sizeof(struct page *)),GFP_KERNEL);
            if (pages == NULL) {
	printk(KERN_WARNING "PULP: Memory allocation failed.\n");
	return -ENOMEM;
      }      
        
      // get pointers to user-space buffers and lock them into memory
      down_read(&current->mm->mmap_sem);
      result = get_user_pages(current, current->mm, addr_start, len, 1, 0, pages, NULL);
      up_read(&current->mm->mmap_sem);
      if (result != len) {
	printk(KERN_WARNING "PULP: Could not get requested user-space virtual addresses.\n");
	printk(KERN_WARNING "Requested %d pages starting at v_addr %#x\n",len,addr_start);
	printk(KERN_WARNING "Obtained  %d pages\n",result);
      }
      
      // virtual to physical address translation
      addr_phys = (unsigned *)kmalloc((size_t)len*sizeof(unsigned),GFP_KERNEL);
      for (i=0;i<len;i++) {
	addr_phys[i] = (unsigned)page_to_phys(pages[i]);
      }
    }
   
    // setup mapping information
    addr_start_vec = (unsigned *)kmalloc((size_t)(len*sizeof(unsigned)),GFP_KERNEL);
    if (addr_start_vec == NULL) {
      printk(KERN_WARNING "PULP: Memory allocation failed.\n");
      return -ENOMEM;
    }
    addr_end_vec = (unsigned *)kmalloc((size_t)(len*sizeof(unsigned)),GFP_KERNEL);
    if (addr_end_vec == NULL) {
      printk(KERN_WARNING "PULP: Memory allocation failed.\n");
      return -ENOMEM;
    }
    addr_offset_vec = (unsigned *)kmalloc((size_t)(len*sizeof(unsigned)),GFP_KERNEL);
    if (addr_offset_vec == NULL) {
      printk(KERN_WARNING "PULP: Memory allocation failed.\n");
      return -ENOMEM;
    }
    page_start_idxs = (unsigned *)kmalloc((size_t)(len*sizeof(unsigned)),GFP_KERNEL);
    if (page_start_idxs == NULL) {
      printk(KERN_WARNING "PULP: Memory allocation failed.\n");
      return -ENOMEM;
    }
    page_end_idxs = (unsigned *)kmalloc((size_t)(len*sizeof(unsigned)),GFP_KERNEL);
    if (page_end_idxs == NULL) {
      printk(KERN_WARNING "PULP: Memory allocation failed.\n");
      return -ENOMEM;
    }      
    addr_start_vec[0] = addr_start;
    addr_end_vec[0] = addr_end;
    page_start_idxs[0] = 0;
    page_end_idxs[0] = 0;

    /*
     *  analyze the physical addresses
     */
    if ( (!const_mapping) & (len > 1) ) {
     	
      // check the number of slices required
      for (i=1;i<len;i++) {	  

	// are the pages also contiguous in physical memory?
	if (addr_phys[i] != (addr_phys[i-1] + PAGE_SIZE)) { // no

	  if (RAB_DEBUG_LEVEL > 1) {
	    printk(KERN_INFO "i = %d\n",i);
	    printk(KERN_INFO "addr_phys[i]   = %#x\n",addr_phys[i]);
	    printk(KERN_INFO "addr_phys[i-1] = %#x\n",addr_phys[i-1]);

	    printk(KERN_INFO "n_slices = %d\n",n_slices);
	    printk(KERN_INFO "addr_start_vec[n_slices-1] = %#x\n",addr_start_vec[n_slices-1]);
	    printk(KERN_INFO "addr_end_vec[n_slices-1]   = %#x\n",addr_end_vec[n_slices-1]);
	  }

	  // finish current slice
	  addr_end_vec[n_slices-1] = (addr_start & 0xFFFFF000) + PAGE_SIZE*i;
	  page_end_idxs[n_slices-1] = i-1;
	  // add a new slice
	  n_slices++;
	  addr_start_vec[n_slices-1] = (addr_start & 0xFFFFF000) + PAGE_SIZE*i;
	  addr_offset_vec[n_slices-1] = addr_phys[i];
	  page_start_idxs[n_slices-1] = i;
	}
	if (i == (len-1)) {
	  // finish last slice
	  addr_end_vec[n_slices-1] = addr_end;
	  page_end_idxs[n_slices-1] = i;
	}
      }
    }
    if (const_mapping) {
      addr_offset_vec[0] = addr_offset;
    }
    else { // const_mapping == false
      addr_offset_vec[0] = addr_phys[0] + (addr_start & 0xFFF); // offset in page
      kfree(addr_phys);
    }

    /*
     *  check for free field in page_ptrs list
     */
    page_ptr_idx = 0;
    for (i=0;i<RAB_N_SLICES*RAB_N_PORTS;i++) {
      if (page_ptr_ref_cntrs[i] == 0) {
	page_ptr_idx = i;
	break;
      }
      if ( i == (RAB_N_SLICES*RAB_N_PORTS) ) {
	printk(KERN_INFO "PULP: No RAB slice available.\n");
	return -EIO;
      }
    }

    /*
     *  setup the slices
     */ 
    // to do: protect with semaphore!!!
    for (i=0;i<n_slices;i++) {

      // search for a free slice
      rab_slice = 0;
      for (j=0;j<RAB_N_SLICES;j++) {
	if (RAB_DEBUG_LEVEL > 1) {
	  printk(KERN_INFO "Testing Slice %d\n",j);
	}
	if ( date_cur > ( BF_GET(rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+j*RAB_TABLE_WIDTH], RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, RAB_CONFIG_N_BITS_DATE))) { // found an expired slice
	  rab_slice = j;
	  break;
	}
	else if (j == (RAB_N_SLICES-1) ) { // no slice free
	  printk(KERN_INFO "PULP: No RAB slice available.\n");
	  return -EIO;
	}
      }
   
      // free memory of slices to be re-configured
      page_ptr_idx_old = rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+2];
      pages_old = page_ptrs[page_ptr_idx_old];
      if (pages_old) { // not used for a constant mapping
	// determine pages to be unlocked
	page_idx_start_old = BF_GET(rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+1],RAB_CONFIG_N_BITS_PAGE, RAB_CONFIG_N_BITS_PAGE);
	page_idx_end_old = BF_GET(rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+1],0,RAB_CONFIG_N_BITS_PAGE);
	// unlock remapped pages
	for (j=page_idx_start_old;j<=page_idx_end_old;j++) {
	  if (RAB_DEBUG_LEVEL > 0) {
	    printk(KERN_INFO "Unlocking Page %d remapped on RAB Slice %d on Port %d.\n",j,rab_slice,rab_port);
	  }
	  if (!PageReserved(pages_old[j])) 
	    SetPageDirty(pages_old[j]);
	  page_cache_release(pages_old[j]);
	  // lower reference counter
	  page_ptr_ref_cntrs[page_ptr_idx_old]--;
	}
	// free memory if no more references exist
	if (!page_ptr_ref_cntrs[page_ptr_idx_old]) {
	  kfree(pages_old);
	  page_ptrs[i*RAB_N_SLICES+j] = 0;
	}
	if (RAB_DEBUG_LEVEL > 0) {
	  printk(KERN_INFO "Number of references to pages pointer = %d.\n",page_ptr_ref_cntrs[page_ptr_idx_old]);
	}
      }
      
      // occupy slice
      rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+0] = request[0];
      BF_SET(rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+1], page_start_idxs[i], RAB_CONFIG_N_BITS_PAGE, RAB_CONFIG_N_BITS_PAGE);
      BF_SET(rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+1], page_end_idxs[i], 0, RAB_CONFIG_N_BITS_PAGE);
      rab_slices[rab_port*RAB_N_SLICES*RAB_TABLE_WIDTH+rab_slice*RAB_TABLE_WIDTH+2] = page_ptr_idx;
      // check that the selected reference list entry is really free = memory has properly been freed
      if ( (!i) & page_ptr_ref_cntrs[page_ptr_idx] ) {
	printk(KERN_WARNING "PULP: Selected reference list entry not free. Number of references = %d.\n",page_ptr_ref_cntrs[page_ptr_idx]);
	return -EIO;
      }
      page_ptr_ref_cntrs[page_ptr_idx]++;
      if (const_mapping) {
	page_ptrs[page_ptr_idx] = 0;
      }
      else {
	page_ptrs[page_ptr_idx] = pages;
      }

      // setup new slice
      offset = 0x10*(rab_port*RAB_N_SLICES+rab_slice);
      iowrite32(addr_start_vec[i],my_dev.rab_config+offset+0x10);
      iowrite32(addr_end_vec[i],my_dev.rab_config+offset+0x14);
      iowrite32(addr_offset_vec[i],my_dev.rab_config+offset+0x18);
      iowrite32(prot,my_dev.rab_config+offset+0x1c);
      
      if (RAB_DEBUG_LEVEL > 0) {
	printk(KERN_INFO "addr_start  %#x\n",addr_start_vec[i]);
	printk(KERN_INFO "addr_end    %#x\n",addr_end_vec[i]);
	printk(KERN_INFO "addr_offset %#x\n",addr_offset_vec[i]);
      }      
      printk(KERN_INFO "PULP: Set up RAB Slice %d on Port %d.\n",rab_slice,rab_port);

      if (!const_mapping) {
	// clean L1 cache lines
	__cpuc_flush_dcache_area((void *)addr_start_vec[i],addr_end_vec[i]-addr_start_vec[i]);

	// clean L2 cache lines 
	outer_cache.flush_range(addr_start_vec[i],addr_end_vec[i]-addr_start_vec[i]);
      }
    
    }
    // kfree
    kfree(addr_start_vec);
    kfree(addr_end_vec);
    kfree(addr_offset_vec);
    kfree(page_start_idxs);
    kfree(page_end_idxs);

    break;

  case PULP_IOCTL_RAB_FREE: // Free RAB slices based on time code
    
    date_cur = BF_GET(arg,0,RAB_CONFIG_N_BITS_DATE);

    // check every slice on every port
    for (i=0;i<RAB_N_PORTS;i++) {
      for (j=0;j<RAB_N_SLICES;j++) {
	date_exp = BF_GET(rab_slices[i*RAB_N_SLICES*RAB_TABLE_WIDTH+j*RAB_TABLE_WIDTH], RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, RAB_CONFIG_N_BITS_DATE);

	if ((date_cur > date_exp) | (date_cur == 0) | (date_cur == BIT_MASK_GEN(RAB_CONFIG_N_BITS_DATE))) {
	  
	  if (RAB_DEBUG_LEVEL > 0) {
	    printk(KERN_INFO "Freeing RAB Slice %d on Port %d.\n",j,i);
	  }
	  
	  page_ptr_idx_old = rab_slices[i*RAB_N_SLICES*RAB_TABLE_WIDTH+j*RAB_TABLE_WIDTH+2];
	  pages_old = page_ptrs[page_ptr_idx_old];
	  if (pages_old) { // not used for a constant mapping
	    // determine pages to be unlocked
	    page_idx_start_old = BF_GET(rab_slices[i*RAB_N_SLICES*RAB_TABLE_WIDTH+j*RAB_TABLE_WIDTH+1],RAB_CONFIG_N_BITS_PAGE, RAB_CONFIG_N_BITS_PAGE);
	    page_idx_end_old = BF_GET(rab_slices[i*RAB_N_SLICES*RAB_TABLE_WIDTH+j*RAB_TABLE_WIDTH+1],0,RAB_CONFIG_N_BITS_PAGE);
	    // unlock remapped pages
	    for (k=page_idx_start_old;k<=page_idx_end_old;k++) {
	      if (RAB_DEBUG_LEVEL > 0) {
		printk(KERN_INFO "Unlocking Page %d remapped on RAB Slice %d on Port %d.\n",k,j,i);
	      }
	      if (!PageReserved(pages_old[k])) 
		SetPageDirty(pages_old[k]);
	      page_cache_release(pages_old[k]);
	      // lower reference counter
	      page_ptr_ref_cntrs[page_ptr_idx_old]--;
	    }
	    // free memory if no more references exist
	    if (!page_ptr_ref_cntrs[page_ptr_idx_old]) {
	      kfree(pages_old);
	      page_ptrs[i*RAB_N_SLICES+j] = 0;
	    }
	    if (RAB_DEBUG_LEVEL > 1) {
	      printk(KERN_INFO "Number of references to pages pointer %d.\n",page_ptr_ref_cntrs[rab_slice]);
	    }
	  }

	  // delete the entries in the table
	  for (k=0;k<RAB_TABLE_WIDTH;k++) {
	    rab_slices[i*RAB_N_SLICES*RAB_TABLE_WIDTH+j*RAB_TABLE_WIDTH+k] = 0;
	  }
	  page_ptr_ref_cntrs[i*RAB_N_SLICES+j] = 0;
	}
      }
    }
    break;

  default:
    return -ENOTTY;
  }

  return retval;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pirmin Vogel");
MODULE_DESCRIPTION("PULPonFPGA driver");
