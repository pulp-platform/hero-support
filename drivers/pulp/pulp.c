//#include <linux/slab.h>
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* KERN_ALERT, container_of */
#include <linux/kdev_t.h>	/* major, minor */
#include <linux/interrupt.h>    /* interrupt handling */
#include <asm/io.h>		/* ioremap, iounmap, iowrite32 */
#include <linux/cdev.h>		/* cdev struct */
#include <linux/fs.h>		/* file_operations struct */
#include <linux/mm.h>           /* vm_area_struct struct */
//#include <asm/uaccess.h>	/* copy_to_user, copy_from_user,access_ok */
#include <linux/time.h>         /* do_gettimeofday */
//#include <linux/ioctl.h>
//#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>        /* udelay */
#include <linux/device.h>       // class_create, device_create

#include "../../lib/inc/zynq.h"
#include "../../lib/inc/pulp_host.h"

// VM_RESERVERD
#ifndef VM_RESERVED
# define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

// methods declarations
int pulp_open(struct inode *inode, struct file *filp);
int pulp_release(struct inode *p_inode, struct file *filp);
ssize_t pulp_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
int pulp_mmap(struct file *filp, struct vm_area_struct *vma);

irqreturn_t pulp_isr_eoc(int irq, void *ptr);
irqreturn_t pulp_isr_rab_miss(int irq, void *ptr);
irqreturn_t pulp_isr_mailbox(int irq, void *ptr);

// important structs
struct file_operations pulp_fops = {
  .owner = THIS_MODULE,
  //.llseek = pulp_llseek,
  .open = pulp_open,
  .release = pulp_release,
  //.read = pulp_read,
  //.write = pulp_write,
  .mmap = pulp_mmap,
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
//static unsigned mailbox_interface0_is;
//static unsigned mailbox_interface1_is;

// methods definitions
/***********************************************************************************
 *
 * init 
 *
 ***********************************************************************************/
static int __init pulp_init(void) {
  
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
  my_dev.l3_mem = ioremap_nocache(L3_MEM_BASE_ADDR, L3_MEM_SIZE_B);
  if (my_dev.l3_mem == NULL) {
    printk(KERN_WARNING "PULP: ioremap_nochache not allowed for non-reserved RAM.\n");
    err = EPERM;
    goto fail_ioremap;
  }
  printk(KERN_INFO "PULP: Shared L3 memory (DRAM) mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.l3_mem);
 
  my_dev.clusters = ioremap_nocache(CLUSTERS_H_BASE_ADDR, CLUSTERS_SIZE_B);
  printk(KERN_INFO "PULP: Clusters mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.clusters); 

  my_dev.soc_periph = ioremap_nocache(SOC_PERIPHERALS_H_BASE_ADDR, SOC_PERIPHERALS_SIZE_B);
  printk(KERN_INFO "PULP: SoC peripherals mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.soc_periph); 

  my_dev.l2_mem = ioremap_nocache(L2_MEM_H_BASE_ADDR, L2_MEM_SIZE_B);
  printk(KERN_INFO "PULP: L2 memory mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.l2_mem); 
 
  my_dev.rab_config = ioremap_nocache(RAB_CONFIG_BASE_ADDR, RAB_CONFIG_SIZE_B);
  printk(KERN_INFO "PULP: Config mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.rab_config); 

  my_dev.gpio = ioremap_nocache(H_GPIO_BASE_ADDR, H_GPIO_SIZE_B);
  printk(KERN_INFO "PULP: Host GPIO mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.gpio); 

  my_dev.slcr = ioremap_nocache(SLCR_BASE_ADDR,SLCR_SIZE_B);
  printk(KERN_INFO "PULP: Zynq SLCR mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.slcr); 

  my_dev.mpcore = ioremap_nocache(MPCORE_BASE_ADDR,MPCORE_SIZE_B);
  printk(KERN_INFO "PULP: Zynq MPCore register mapped to virtual kernel space @ %#lx.\n",(long unsigned int) my_dev.mpcore); 

  // /*********************
  //  *
  //  * interrupts
  //  *
  //  *********************/
  // // configure interrupt sensitivities
  // // read configuration
  // mpcore_icdicfr3=ioread32(my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B);
  // mpcore_icdicfr4=ioread32(my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B);
  // 
  // // change configuration for eoc and RAB miss
  // mpcore_icdicfr3 &= 0x03FFFFFF; // delete bits 31 - 26
  // mpcore_icdicfr3 |= 0x7C000000; // set bits 31 - 26: 01 11 11
  // 
  // mpcore_icdicfr4 &= 0xFFFFFFFC; // delete bits 1 - 0
  // mpcore_icdicfr4 |= 0x00000001; // set bits 1 - 0: 01
  //   
  // // write configuration
  // iowrite32(mpcore_icdicfr3,my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B);
  // iowrite32(mpcore_icdicfr4,my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B);
  // 
  // // request interrupts and install handlers
  // // eoc
  // err = request_irq(END_OF_COMPUTATION_IRQ, pulp_isr_eoc, 0, "PULP", NULL);
  // if (err) {
  //   printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
  //   goto fail_request_irq;
  // }
  // 
  // // RAB miss
  // err = request_irq(RAB_MISS_IRQ, pulp_isr_rab_miss, 0, "PULP", NULL);
  // if (err) {
  //   printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
  //   goto fail_request_irq;
  // }
  // 
  // // mailbox interface 0
  // err = request_irq(MAILBOX_INTERFACE0_IRQ, pulp_isr_mailbox, 0, "PULP", NULL);
  // if (err) {
  //   printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
  //   goto fail_request_irq;
  // }
  // 
  // // mailbox interface 1
  // err = request_irq(MAILBOX_INTERFACE1_IRQ, pulp_isr_mailbox, 0, "PULP", NULL);
  // if (err) {
  //   printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
  //   goto fail_request_irq;
  // }

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
  // free_irq(END_OF_COMPUTATION_IRQ,NULL);
  // free_irq(RAB_MISS_IRQ,NULL);
  // free_irq(MAILBOX_INTERFACE0_IRQ,NULL);
  // free_irq(MAILBOX_INTERFACE1_IRQ,NULL);
  //fail_request_irq:
  iounmap(my_dev.l3_mem);
  iounmap(my_dev.clusters);
  iounmap(my_dev.soc_periph);
  iounmap(my_dev.l2_mem);
  iounmap(my_dev.rab_config);
  iounmap(my_dev.gpio);
  iounmap(my_dev.slcr);
  iounmap(my_dev.mpcore);
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
  // free_irq(END_OF_COMPUTATION_IRQ,NULL);
  // free_irq(RAB_MISS_IRQ,NULL);
  // free_irq(MAILBOX_INTERFACE0_IRQ,NULL);
  // free_irq(MAILBOX_INTERFACE1_IRQ,NULL);
  iounmap(my_dev.l3_mem);
  iounmap(my_dev.clusters);
  iounmap(my_dev.soc_periph);
  iounmap(my_dev.l2_mem);
  iounmap(my_dev.rab_config);
  iounmap(my_dev.gpio);
  iounmap(my_dev.slcr);
  iounmap(my_dev.mpcore);
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
  if (off < L3_MEM_SIZE_B) {
    // L3
    base_addr = L3_MEM_BASE_ADDR;
    size_b = L3_MEM_SIZE_B;
    strcpy(type,"L3");
  }
  else if (off < L3_MEM_SIZE_B + CLUSTERS_SIZE_B) {
    // Clusters
    base_addr = CLUSTERS_H_BASE_ADDR;
    off = off - L3_MEM_SIZE_B;
    size_b = CLUSTERS_SIZE_B;
    strcpy(type,"Clusters");
  }
  else if (off < (L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B)) {
    // SoC peripherals
    base_addr = SOC_PERIPHERALS_H_BASE_ADDR;
    off = off - L3_MEM_SIZE_B - CLUSTERS_SIZE_B;
    size_b = SOC_PERIPHERALS_SIZE_B;
    strcpy(type,"Peripherals");
  }
  else if (off < (L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B)) {
    // L2
    base_addr = L2_MEM_H_BASE_ADDR;
    off = off - L3_MEM_SIZE_B - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B;
    size_b = L2_MEM_SIZE_B;
    strcpy(type,"L2");
  }
  else if (off < (L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B + RAB_CONFIG_SIZE_B)) {
    // RAB config
    base_addr = RAB_CONFIG_BASE_ADDR;
    off = off - L3_MEM_SIZE_B - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - L2_MEM_SIZE_B;
    size_b = RAB_CONFIG_SIZE_B;
    strcpy(type,"RAB config");
  }
  else if (off < (L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B + RAB_CONFIG_SIZE_B + H_GPIO_SIZE_B)) {
    // GPIO
    base_addr = H_GPIO_BASE_ADDR;
    off = off - L3_MEM_SIZE_B - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - L2_MEM_SIZE_B - RAB_CONFIG_SIZE_B;
    size_b = H_GPIO_SIZE_B;
    strcpy(type,"GPIO");
  }
  else if (off < (L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B + RAB_CONFIG_SIZE_B + H_GPIO_SIZE_B + SLCR_SIZE_B)) {
    // SLCR
    base_addr = SLCR_BASE_ADDR;
    off = off - L3_MEM_SIZE_B - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - L2_MEM_SIZE_B - RAB_CONFIG_SIZE_B - H_GPIO_SIZE_B;
    size_b = SLCR_SIZE_B;
    strcpy(type,"Zynq SLCR");
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

// /***********************************************************************************
//  *
//  * interrupt service routines 
//  *
//  ***********************************************************************************/
// irqreturn_t pulp_isr_eoc(int irq, void *ptr) {
//   
//   // do something
//   do_gettimeofday(&time);
//   printk(KERN_INFO "PULP: End of Computation: %02li:%02li:%02li.\n",(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
// 
//   // interrupt is just a pulse, no need to clear it
// 
//   return IRQ_HANDLED;
// }
// 
// irqreturn_t pulp_isr_rab_miss(int irq, void *ptr) {
//     
//   do_gettimeofday(&time);
//   printk(KERN_INFO "PULP: RAB miss interrupt handled at %02li:%02li:%02li.\n",(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
//   // read FPGA reset control register
//   slcr_value = ioread32(my_dev.slcr+SLCR_FPGA_RST_CTRL_OFFSET_B);
//   // extract the FPGA_OUT_RST bits
//   slcr_value = slcr_value & 0xF;
//   // enable reset
//   iowrite32(slcr_value | (0x1 << SLCR_FPGA_OUT_RST),my_dev.slcr+SLCR_FPGA_RST_CTRL_OFFSET_B);
//   // wait
//   udelay(10);
//   // disable reset
//   iowrite32(slcr_value & (0xF & (0x0 << SLCR_FPGA_OUT_RST)),my_dev.slcr+SLCR_FPGA_RST_CTRL_OFFSET_B);
//   printk(KERN_INFO "PULP: Device has been reset due to a RAB miss.\n");
// 
//   return IRQ_HANDLED;
// }
// 
// irqreturn_t pulp_isr_mailbox(int irq, void *ptr) {
//   // do something
//   do_gettimeofday(&time);
// 
//   // clear the interrupt
//   if (MAILBOX_INTERFACE0_IRQ == irq) {
//     mailbox_interface0_is = 0x7 & ioread32(my_dev.clusters+MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_IS_OFFSET_B);
//     iowrite32(0x7,my_dev.clusters+MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_IS_OFFSET_B);
//     printk(KERN_INFO "PULP: Mailbox Interface 0 interrupt status: %#x. Interrupt handled at: %02li:%02li:%02li.\n",mailbox_interface0_is,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
//   }
//   else {// MAILBOX_INTERFACE1_IRQ
//     mailbox_interface1_is = 0x7 & ioread32(my_dev.clusters+MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_IS_OFFSET_B);
//     iowrite32(0x7,my_dev.clusters+MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_IS_OFFSET_B);
//     printk(KERN_INFO "PULP: Mailbox Interface 1 interrupt status: %#x. Interrupt handled at: %02li:%02li:%02li.\n",mailbox_interface1_is,(time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
//   }
// 
//   return IRQ_HANDLED;
// }

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pirmin Vogel");
MODULE_DESCRIPTION("PULPonFPGA driver");
