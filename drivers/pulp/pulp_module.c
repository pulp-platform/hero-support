/* --=========================================================================--
 *
 * ██████╗ ██╗   ██╗██╗     ██████╗     ██████╗ ██████╗ ██╗██╗   ██╗███████╗██████╗ 
 * ██╔══██╗██║   ██║██║     ██╔══██╗    ██╔══██╗██╔══██╗██║██║   ██║██╔════╝██╔══██╗
 * ██████╔╝██║   ██║██║     ██████╔╝    ██║  ██║██████╔╝██║██║   ██║█████╗  ██████╔╝
 * ██╔═══╝ ██║   ██║██║     ██╔═══╝     ██║  ██║██╔══██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗
 * ██║     ╚██████╔╝███████╗██║         ██████╔╝██║  ██║██║ ╚████╔╝ ███████╗██║  ██║
 * ╚═╝      ╚═════╝ ╚══════╝╚═╝         ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝
 *                                                                                  
 * 
 * Author: Pirmin Vogel - vogelpi@iis.ee.ethz.ch
 * 
 * Purpose : Linux kernel-level driver for PULPonFPGA
 *           RAB management for shared virtual memory between host and PULP
 * 
 * --=========================================================================--
 *
 * Copyright (C) 2016 Pirmin Vogel <vogelpi@iis.ee.ethz.ch>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 1, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
 * 02110-1301 USA.
 * 
 */

/***************************************************************************************/

#include <linux/module.h>     /* Needed by all modules */
#include <linux/kernel.h>     /* KERN_ALERT, container_of */
#include <linux/kdev_t.h>     /* major, minor */
#include <linux/interrupt.h>  /* interrupt handling */
#include <linux/workqueue.h>  /* schedule non-atomic work */
#include <linux/wait.h>       /* wait_queue */
#include <asm/io.h>           /* ioremap, iounmap, iowrite32 */
#include <linux/cdev.h>       /* cdev struct */
#include <linux/fs.h>         /* file_operations struct */
#include <linux/mm.h>         /* vm_area_struct struct, page struct, PAGE_SHIFT, page_to_phys */
#include <linux/pagemap.h>    /* page_cache_release() */
#include <asm/cacheflush.h>   /* __cpuc_flush_dcache_area, outer_cache.flush_range */
#include <asm/uaccess.h>      /* copy_to_user, copy_from_user, access_ok */
#include <linux/time.h>       /* do_gettimeofday */
#include <linux/ioctl.h>      /* ioctl */
#include <linux/slab.h>       /* kmalloc */
#include <linux/errno.h>      /* errno */
#include <linux/sched.h>      /* wake_up_interruptible(), TASK_INTERRUPTIBLE */
#include <linux/delay.h>      /* udelay */
#include <linux/device.h>     // class_create, device_create

/***************************************************************************************/
//#include <linux/wait.h>
#include <linux/ktime.h>      /* For ktime_get(), ktime_us_delta() */
/***************************************************************************************/

#include "pulp_module.h"
#include "pulp_mem.h"
#include "pulp_rab.h"
#include "pulp_dma.h"
#include "pulp_mbox.h" 

#if PLATFORM == JUNO
  #include <linux/platform_device.h> /* for device tree stuff*/
  #include <linux/device.h>
  #include <linux/of_device.h>
  #include <asm/compat.h>                /* for compat_ioctl */
#endif

/***************************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pirmin Vogel");
MODULE_DESCRIPTION("PULPonFPGA driver");

/***************************************************************************************/

// Device Tree (for Juno) {{{
#if PLATFORM == JUNO
  /***********************************************************************************
   *
   * ██████╗ ███████╗██╗   ██╗██╗ ██████╗███████╗    ████████╗██████╗ ███████╗███████╗
   * ██╔══██╗██╔════╝██║   ██║██║██╔════╝██╔════╝    ╚══██╔══╝██╔══██╗██╔════╝██╔════╝
   * ██║  ██║█████╗  ██║   ██║██║██║     █████╗         ██║   ██████╔╝█████╗  █████╗  
   * ██║  ██║██╔══╝  ╚██╗ ██╔╝██║██║     ██╔══╝         ██║   ██╔══██╗██╔══╝  ██╔══╝  
   * ██████╔╝███████╗ ╚████╔╝ ██║╚██████╗███████╗       ██║   ██║  ██║███████╗███████╗
   * ╚═════╝ ╚══════╝  ╚═══╝  ╚═╝ ╚═════╝╚══════╝       ╚═╝   ╚═╝  ╚═╝╚══════╝╚══════╝
   *
   * On JUNO, the IRQ is allocated through the device tree at boot time. In order to
   * register the interrupt service routine, request_irq() needs the interrupt index
   * reserved at boot time instead of the physical IRQ. This can be obtained through 
   * the device tree. The addresses we still "pass" through the header files. 
   *
   * On ZYNQ, a simplified IRQ allocation is used. The interrupt index corresponds to
   * the physical IRQ known at compile time. The device tree is not needed.
   *
   ***********************************************************************************/                                                                                 
  
  // static variables
  static int intr_reg_irq; 

  // connect to the device tree
  static struct of_device_id pulp_of_match[] = {
    {
      .compatible = "pulp,pulpjuno",
    }, { /* sentinel */}
  };
  
  MODULE_DEVICE_TABLE(of, pulp_of_match);

  // method definition
  static int pulp_probe(struct platform_device *pdev)
  {
    printk(KERN_ALERT "PULP: Probing device.\n");

    intr_reg_irq = platform_get_irq(pdev,0);
    if (intr_reg_irq <= 0) {
      printk(KERN_WARNING "PULP: Could not allocate IRQ resource.\n");
      return -ENODEV;
    }

    return 0;
  }

  static int pulp_remove(struct platform_device *pdev)
  {
    printk(KERN_ALERT "PULP: Removing device.\n");

    return 0;
  }

  // (un-)register to/from the device tree
  static struct platform_driver pulp_platform_driver = {
    .probe  = pulp_probe,
    .remove = pulp_remove,
    .driver = {
      .name = "pulp",
      .owner = THIS_MODULE,
      .of_match_table = pulp_of_match,
      },
  };

/***************************************************************************************/
#endif
// }}}

// VM_RESERVERD for mmap
#ifndef VM_RESERVED
  #define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

// method declarations {{{
static int  pulp_open   (struct inode *inode, struct file *filp);
static int  pulp_release(struct inode *p_inode, struct file *filp);
static int  pulp_mmap   (struct file *filp, struct vm_area_struct *vma);
static long pulp_ioctl  (struct file *filp, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
  static long pulp_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif

#if PLATFORM == JUNO
  static irqreturn_t pulp_isr        (int irq, void *ptr);
#else
  static irqreturn_t pulp_isr_eoc (int irq, void *ptr);
  static irqreturn_t pulp_isr_mbox(int irq, void *ptr);
  static irqreturn_t pulp_isr_rab (int irq, void *ptr);
#endif // PLATFORM == JUNO

static void pulp_rab_handle_miss(unsigned unused);
// }}}

// important structs {{{
struct file_operations pulp_fops = {
  .owner          = THIS_MODULE,
  .open           = pulp_open,
  .release        = pulp_release,
  .read           = pulp_mbox_read,
  .mmap           = pulp_mmap,
  .unlocked_ioctl = pulp_ioctl,
#ifdef CONFIG_COMPAT
  .compat_ioctl = pulp_compat_ioctl,
#endif
};
// }}}

// static variables {{{
static PulpDev my_dev;

static struct class *my_class; 

static unsigned pulp_cluster_select = 0;

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
static unsigned rab_mh_acp = 0;
static unsigned rab_mh_lvl = 0;
#ifdef PROFILE_RAB_MH
  static unsigned clk_cntr_resp_tmp[N_CORES_TIMER];
  static unsigned clk_cntr_resp[N_CORES_TIMER];
  static unsigned clk_cntr_sched[N_CORES_TIMER];
  static unsigned clk_cntr_refill[N_CORES_TIMER];
  static unsigned n_misses = 0;
  static unsigned n_first_misses = 0;
#endif

// for DMA
static struct dma_chan * pulp_dma_chan[2];
static DmaCleanup pulp_dma_cleanup[2];
// }}}

// methods definitions

// init {{{
/***********************************************************************************
 *
 * ██╗███╗   ██╗██╗████████╗
 * ██║████╗  ██║██║╚══██╔══╝
 * ██║██╔██╗ ██║██║   ██║   
 * ██║██║╚██╗██║██║   ██║   
 * ██║██║ ╚████║██║   ██║   
 * ╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝    
 *
 ***********************************************************************************/
static int __init pulp_init(void)
{
  int err;
  unsigned gpio;
  #if PLATFORM != JUNO
    unsigned mpcore_icdicfr3, mpcore_icdicfr4;
  #endif

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
  my_dev.mbox = ioremap_nocache(MBOX_H_BASE_ADDR, MBOX_SIZE_B*2);
  printk(KERN_INFO "PULP: Mailbox mapped to virtual kernel space @ %#lx.\n",
    (long unsigned int) my_dev.mbox);
  pulp_mbox_init(my_dev.mbox);

  #if PLATFORM == JUNO 
    my_dev.intr_reg = ioremap_nocache(INTR_REG_BASE_ADDR,INTR_REG_SIZE_B);
    printk(KERN_INFO "PULP: Interrupt register mapped to virtual kernel space @ %#lx.\n",
      (long unsigned int) my_dev.intr_reg);

    my_dev.rab_ar_log = ioremap_nocache(RAB_AR_LOG_BASE_ADDR, RAB_AX_LOG_SIZE_B);
    printk(KERN_INFO "PULP: RAB AR log mapped to virtual kernel space @ %#lx.\n",
      (long unsigned int) my_dev.rab_ar_log);

    my_dev.rab_aw_log = ioremap_nocache(RAB_AW_LOG_BASE_ADDR, RAB_AX_LOG_SIZE_B);
    printk(KERN_INFO "PULP: RAB AW log mapped to virtual kernel space @ %#lx.\n",
      (long unsigned int) my_dev.rab_aw_log);

  #else // PLATFORM != JUNO 
    my_dev.slcr = ioremap_nocache(SLCR_BASE_ADDR,SLCR_SIZE_B);
    printk(KERN_INFO "PULP: Zynq SLCR mapped to virtual kernel space @ %#lx.\n",
      (long unsigned int) my_dev.slcr);
  
    // make sure to enable the PL clock
    if ( !BF_GET(ioread32((void *)((unsigned long)my_dev.slcr+SLCR_FPGA0_THR_STA_OFFSET_B)),16,1) ) {
      iowrite32(0x0,(void *)((unsigned long)my_dev.slcr+SLCR_FPGA0_THR_CNT_OFFSET_B));
  
      if ( !BF_GET(ioread32((void *)((unsigned long)my_dev.slcr+SLCR_FPGA0_THR_STA_OFFSET_B)),16,1) ) {
        printk(KERN_WARNING "PULP: Could not enable reference clock for PULP.\n");
        goto fail_ioremap;
      }
    }
   
    my_dev.mpcore = ioremap_nocache(MPCORE_BASE_ADDR,MPCORE_SIZE_B);
    printk(KERN_INFO "PULP: Zynq MPCore register mapped to virtual kernel space @ %#lx.\n",
      (long unsigned int) my_dev.mpcore);

    my_dev.uart0 = ioremap_nocache(UART0_BASE_ADDR,UART0_SIZE_B);
    printk(KERN_INFO "PULP: Zynq UART0 control register mapped to virtual kernel space @ %#lx.\n",
      (long unsigned int) my_dev.uart0);

    // make sure to enable automatic flow control on PULP -> Host UART
    iowrite32(0x20,(void *)((unsigned long)my_dev.uart0+MODEM_CTRL_REG0_OFFSET_B));

  #endif // PLATFORM == JUNO

  my_dev.gpio = ioremap_nocache(H_GPIO_BASE_ADDR, H_GPIO_SIZE_B);
  printk(KERN_INFO "PULP: Host GPIO mapped to virtual kernel space @ %#lx.\n",
    (long unsigned int) my_dev.gpio); 
  
  // remove GPIO reset
  gpio = 0;
  BIT_SET(gpio,BF_MASK_GEN(GPIO_RST_N,1));
  BIT_SET(gpio,BF_MASK_GEN(GPIO_CLK_EN,1));
  iowrite32(gpio,(void *)((unsigned long)my_dev.gpio+0x8));
  
  // RAB config  
  my_dev.rab_config = ioremap_nocache(RAB_CONFIG_BASE_ADDR, RAB_CONFIG_SIZE_B);
  printk(KERN_INFO "PULP: RAB config mapped to virtual kernel space @ %#lx.\n",
    (long unsigned int) my_dev.rab_config); 
  err = pulp_rab_init(&my_dev);
  if (err) {
    printk(KERN_WARNING "PULP: RAB initialization failed.\n");
    goto fail_ioremap;
  }

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

  // for profiling
  my_dev.l3_mem = ioremap_nocache(L3_MEM_H_BASE_ADDR, L3_MEM_SIZE_B);
  if (my_dev.l3_mem == NULL) {
    printk(KERN_WARNING "PULP: ioremap_nocache not allowed for non-reserved RAM.\n");
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

  #if PLATFORM == JUNO

    // register the device to get the interrupt index
    err = platform_driver_register(&pulp_platform_driver);
    if (err) {
      printk(KERN_WARNING "PULP: Error registering platform driver: %d\n",err);
      goto fail_request_irq;
    }

    // request interrupts and install top-half handler
    err = request_irq(intr_reg_irq, pulp_isr, 0 , "PULP", NULL);
    if (err) {
      printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
      goto fail_request_irq;
    }

  #else // PLATFORM != JUNO
    // configure interrupt sensitivities: Zynq-7000 Technical Reference Manual, Section 7.2.4
    // read configuration
    mpcore_icdicfr3=ioread32((void *)((unsigned long)my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B));
    mpcore_icdicfr4=ioread32((void *)((unsigned long)my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B));
     
    // configure rising-edge active for 61-65
    mpcore_icdicfr3 &= 0x03FFFFFF; // delete bits 31 - 26
    mpcore_icdicfr3 |= 0xFC000000; // set bits 31 - 26: 11 11 11
      
    mpcore_icdicfr4 &= 0xFFFFFFF0; // delete bits 3 - 0
    mpcore_icdicfr4 |= 0x0000000F; // set bits 3 - 0: 11 11
       
    // write configuration
    iowrite32(mpcore_icdicfr3,(void *)((unsigned long)my_dev.mpcore+MPCORE_ICDICFR3_OFFSET_B));
    iowrite32(mpcore_icdicfr4,(void *)((unsigned long)my_dev.mpcore+MPCORE_ICDICFR4_OFFSET_B));
    
    // request interrupts and install handlers
    // eoc
    err = request_irq(END_OF_COMPUTATION_IRQ, pulp_isr_eoc, 0, "PULP", NULL);
    if (err) {
      printk(KERN_WARNING "PULP: Error requesting IRQ.\n");
      goto fail_request_irq;
    }
    
    // mailbox
    err = request_irq(MBOX_IRQ, pulp_isr_mbox, 0, "PULP", NULL);
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
  #endif // PLATFORM != JUNO

  /************************************
   *
   *  request DMA channels
   *
   ************************************/
  #if PLATFORM != JUNO
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
  #endif // PLATFORM != JUNO

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
    #if PLATFORM != JUNO
      pulp_dma_chan_clean(pulp_dma_chan[1]);
      pulp_dma_chan_clean(pulp_dma_chan[0]);
  fail_request_dma:
    #endif // PLATFORM != JUNO
    #if PLATFORM == JUNO
      free_irq(intr_reg_irq,NULL);
      platform_driver_unregister(&pulp_platform_driver);
    #else // PLATFORM != JUNO
      free_irq(END_OF_COMPUTATION_IRQ,NULL);
      free_irq(MBOX_IRQ,NULL);
      free_irq(RAB_MISS_IRQ,NULL);
      free_irq(RAB_MULTI_IRQ,NULL);
      free_irq(RAB_PROT_IRQ,NULL);
    #endif // PLATFORM == JUNO
  fail_request_irq:
    #if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
      pulp_rab_prof_free();
    #endif
    iounmap(my_dev.rab_config);
    iounmap(my_dev.mbox);
    #if PLATFORM == JUNO
      pulp_rab_ax_log_free();
      iounmap(my_dev.intr_reg);
      iounmap(my_dev.rab_ar_log);
      iounmap(my_dev.rab_aw_log);
    #else // PLATFORM != JUNO
      iounmap(my_dev.slcr);
      iounmap(my_dev.mpcore);
      iounmap(my_dev.uart0);
    #endif // PLATFORM == JUNO
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
// }}}

// exit {{{
/***********************************************************************************
 *
 * ███████╗██╗  ██╗██╗████████╗
 * ██╔════╝╚██╗██╔╝██║╚══██╔══╝
 * █████╗   ╚███╔╝ ██║   ██║   
 * ██╔══╝   ██╔██╗ ██║   ██║   
 * ███████╗██╔╝ ██╗██║   ██║   
 * ╚══════╝╚═╝  ╚═╝╚═╝   ╚═╝   
 * 
 ***********************************************************************************/
static void __exit pulp_exit(void)
{
  pulp_mbox_clear();

  printk(KERN_ALERT "PULP: Unloading device driver.\n");
  // undo __init pulp_init
  #if PLATFORM == JUNO
    free_irq(intr_reg_irq,NULL);
    platform_driver_unregister(&pulp_platform_driver);
  #else // PLATFORM != JUNO
    pulp_dma_chan_clean(pulp_dma_chan[1]);
    pulp_dma_chan_clean(pulp_dma_chan[0]);
    free_irq(END_OF_COMPUTATION_IRQ,NULL);
    free_irq(MBOX_IRQ,NULL);
    free_irq(RAB_MISS_IRQ,NULL);
    free_irq(RAB_MULTI_IRQ,NULL);
    free_irq(RAB_PROT_IRQ,NULL);
  #endif // PLATFORM == JUNO
  #if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
    pulp_rab_prof_free();
  #endif
  iounmap(my_dev.rab_config);
  iounmap(my_dev.mbox);
  #if PLATFORM == JUNO
    pulp_rab_ax_log_free();
    iounmap(my_dev.intr_reg);
    iounmap(my_dev.rab_ar_log);
    iounmap(my_dev.rab_aw_log);
  #else // PLATFORM != JUNO
    iounmap(my_dev.slcr);
    iounmap(my_dev.mpcore);
    iounmap(my_dev.uart0);
  #endif // PLATFORM == JUNO
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
// }}}

// open {{{
/***********************************************************************************
 *
 *  ██████╗ ██████╗ ███████╗███╗   ██╗
 * ██╔═══██╗██╔══██╗██╔════╝████╗  ██║
 * ██║   ██║██████╔╝█████╗  ██╔██╗ ██║
 * ██║   ██║██╔═══╝ ██╔══╝  ██║╚██╗██║
 * ╚██████╔╝██║     ███████╗██║ ╚████║
 *  ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═══╝
 * 
 ***********************************************************************************/

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
// }}}

// release {{{
/***********************************************************************************
 *
 * ██████╗ ███████╗██╗     ███████╗ █████╗ ███████╗███████╗
 * ██╔══██╗██╔════╝██║     ██╔════╝██╔══██╗██╔════╝██╔════╝
 * ██████╔╝█████╗  ██║     █████╗  ███████║███████╗█████╗  
 * ██╔══██╗██╔══╝  ██║     ██╔══╝  ██╔══██║╚════██║██╔══╝  
 * ██║  ██║███████╗███████╗███████╗██║  ██║███████║███████╗
 * ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝
 * 
 ***********************************************************************************/

int pulp_release(struct inode *p_inode, struct file *filp)
{
  pulp_mbox_clear();

  printk(KERN_INFO "PULP: Device released.\n");
 
  return 0;
}
// }}}

// mmap {{{
/***********************************************************************************
 *
 * ███╗   ███╗███╗   ███╗ █████╗ ██████╗ 
 * ████╗ ████║████╗ ████║██╔══██╗██╔══██╗
 * ██╔████╔██║██╔████╔██║███████║██████╔╝
 * ██║╚██╔╝██║██║╚██╔╝██║██╔══██║██╔═══╝ 
 * ██║ ╚═╝ ██║██║ ╚═╝ ██║██║  ██║██║     
 * ╚═╝     ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝     
 *
 ***********************************************************************************/
int pulp_mmap(struct file *filp, struct vm_area_struct *vma)
{
  unsigned long base_addr; // base address to use for the remapping
  unsigned long size_b;    // size for the remapping
  unsigned long off;       // address offset in VMA
  unsigned long physical;  // PFN of physical page
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
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B)) {
    // Mailbox
    base_addr = MBOX_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B;
    size_b = MBOX_SIZE_B;
    strcpy(type,"Mailbox");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B)) {
    // L2
    base_addr = L2_MEM_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B;
    size_b = L2_MEM_SIZE_B;
    strcpy(type,"L2");
  }
  // Platform
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B)) {
    // Shared L3
    base_addr = L3_MEM_H_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B - L2_MEM_SIZE_B;
    size_b = L3_MEM_SIZE_B;
    strcpy(type,"Shared L3");
  }
  // PULP external, PULPEmu 
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B)) {
    // H_GPIO
    base_addr = H_GPIO_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B;
    size_b = H_GPIO_SIZE_B;
    strcpy(type,"Host GPIO");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B)) {
    // CLKING
    base_addr = CLKING_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B;
    size_b = CLKING_SIZE_B;
    strcpy(type,"Clking");
  }
  else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                  + H_GPIO_SIZE_B + CLKING_SIZE_B + RAB_CONFIG_SIZE_B)) {
    // RAB config
    base_addr = RAB_CONFIG_BASE_ADDR;
    off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
      - H_GPIO_SIZE_B - CLKING_SIZE_B;
    size_b = RAB_CONFIG_SIZE_B;
    strcpy(type,"RAB config");
  }
  #if PLATFORM != JUNO
    // Zynq System
    else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                    + H_GPIO_SIZE_B + CLKING_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B)) {
      // Zynq SLCR
      base_addr = SLCR_BASE_ADDR;
      off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
        - H_GPIO_SIZE_B - CLKING_SIZE_B - RAB_CONFIG_SIZE_B;
      size_b = SLCR_SIZE_B;
      strcpy(type,"Zynq SLCR");
    }
    else if (off < (CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B 
                    + H_GPIO_SIZE_B + CLKING_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B 
                    + MPCORE_SIZE_B)) {
      // Zynq MPCore
      base_addr = MPCORE_BASE_ADDR;
      off = off - CLUSTERS_SIZE_B - SOC_PERIPHERALS_SIZE_B - MBOX_SIZE_B - L2_MEM_SIZE_B - L3_MEM_SIZE_B 
        - H_GPIO_SIZE_B - CLKING_SIZE_B - RAB_CONFIG_SIZE_B - SLCR_SIZE_B;
      size_b = MPCORE_SIZE_B;
      strcpy(type,"Zynq MPCore");
    }
  #endif // PLATFORM != JUNO
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
    "PULP: %s memory mapped. \nPhysical address = %#lx, user-space virtual address = %#lx, vsize = %#lx.\n",
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

// }}}

// isr {{{
/***********************************************************************************
 *
 * ██╗███████╗██████╗ 
 * ██║██╔════╝██╔══██╗
 * ██║███████╗██████╔╝
 * ██║╚════██║██╔══██╗
 * ██║███████║██║  ██║
 * ╚═╝╚══════╝╚═╝  ╚═╝
 *
 ***********************************************************************************/
#if PLATFORM == JUNO
  irqreturn_t pulp_isr(int irq, void *ptr)
  {
    unsigned long intr_reg_value;
    struct timeval time;
    int i;

    // read and clear the interrupt register
    intr_reg_value = ioread64((void *)(unsigned long)my_dev.intr_reg);
    //printk(KERN_INFO "PULP: Interrupt status %#lx \n",intr_reg_value);  

   /*********************
    *
    * top-half handler
    *
    *********************/
    do_gettimeofday(&time);

    if ( BF_GET(intr_reg_value,INTR_MBOX,1) ) { // mailbox
      pulp_mbox_intr(my_dev.mbox);
    }
    if ( BF_GET(intr_reg_value,INTR_RAB_MISS,1) ) { // RAB miss
      if (rab_mh) { 
#ifdef PROFILE_RAB_MH
        // read PE timers
        for (i=0; i<N_CORES_TIMER; i++) {
          clk_cntr_resp_tmp[i] = ioread32((void *)((unsigned long)my_dev.clusters
            +TIMER_GET_TIME_LO_OFFSET_B+(i+1)*PE_TIMER_OFFSET_B));
        }
#endif
        schedule_work(&rab_mh_w);
      }

     if ( (DEBUG_LEVEL_RAB_MH > 1) || (0 == rab_mh) ) {
      printk(KERN_INFO "PULP: RAB miss interrupt handled at %02li:%02li:%02li.\n",
        (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
  
      // for debugging
      //pulp_rab_mapping_print(my_dev.rab_config,0xAAAA);
      }
    }
    if ( BF_GET(intr_reg_value,INTR_RAB_MHR_FULL,1) ) { // RAB mhr full
      printk(KERN_ALERT "PULP: RAB mhr full interrupt received at %02li:%02li:%02li.\n",
        (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
    } 
    if ( BF_GET(intr_reg_value,INTR_RAB_MULTI,1) ) { // RAB multi
      printk(KERN_ALERT "PULP: RAB multi interrupt received at %02li:%02li:%02li.\n",
        (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
    }
    if ( BF_GET(intr_reg_value,INTR_RAB_PROT,1) ) { // RAB prot
      printk(KERN_ALERT "PULP: RAB prot interrupt received at %02li:%02li:%02li.\n",
        (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
    } 
    if ( BF_GET(intr_reg_value, INTR_EOC_0, INTR_EOC_N-INTR_EOC_0+1) ) { // EOC
      for (i=0; i<N_CLUSTERS; i++) {
        if ( BF_GET(intr_reg_value, INTR_EOC_0+i, 1) ) {
          printk(KERN_INFO "PULP: End of Computation Cluster %i at %02li:%02li:%02li.\n",
            i, (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
        }
      }

      pulp_rab_ax_log_read(pulp_cluster_select, 1);

    }
    if ( BF_GET(intr_reg_value, INTR_RAB_AR_LOG_FULL, 1)
      || BF_GET(intr_reg_value, INTR_RAB_AW_LOG_FULL, 1) ) {
      printk(KERN_INFO "PULP: RAB AX log full interrupt received at %02li:%02li:%02li.\n",
        (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);

      pulp_rab_ax_log_read(pulp_cluster_select, 1);
    }   
  
    return IRQ_HANDLED;
  }

#else // PLATFORM != JUNO
  irqreturn_t pulp_isr_eoc(int irq, void *ptr)
  { 
    struct timeval time;

    // do something
    do_gettimeofday(&time);
    printk(KERN_INFO "PULP: End of Computation: %02li:%02li:%02li.\n",
           (time.tv_sec / 3600) % 24, (time.tv_sec / 60) % 60, time.tv_sec % 60);
   
    // interrupt is just a pulse, no need to clear it
    return IRQ_HANDLED;
  }
   
  irqreturn_t pulp_isr_mbox(int irq, void *ptr)
  {
    pulp_mbox_intr(my_dev.mbox);
  
    return IRQ_HANDLED;
  }
  
  irqreturn_t pulp_isr_rab(int irq, void *ptr)
  { 
    struct timeval time;
    char rab_interrupt_type[9];

    // detect RAB interrupt type
    if (RAB_MISS_IRQ == irq)
      strcpy(rab_interrupt_type,"miss");
    else if (RAB_MULTI_IRQ == irq)
      strcpy(rab_interrupt_type,"multi");
    else if (RAB_PROT_IRQ == irq)
      strcpy(rab_interrupt_type,"prot");
    else // RAB_MHR_FULL_IRQ == irq
      strcpy(rab_interrupt_type,"mhr full");
  
    if (RAB_MISS_IRQ == irq) {
      if (rab_mh) {
  
  #ifdef PROFILE_RAB_MH
        // read PE timers
        int i;
        for (i=0; i<N_CORES_TIMER; i++) {
          clk_cntr_resp_tmp[i] = ioread32((void *)((unsigned long)my_dev.clusters
            + TIMER_GET_TIME_LO_OFFSET_B + (i+1)*PE_TIMER_OFFSET_B));
        }
  #endif
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
      pulp_rab_mapping_print(my_dev.rab_config,0xAAAA);

      if (RAB_MULTI_IRQ == irq){
        pulp_rab_l2_print_valid_entries(1);
      }
    }
  
    return IRQ_HANDLED;
  }

#endif // PLATFORM == JUNO
// }}}

// ioctl {{{
/***********************************************************************************
 *
 * ██╗ ██████╗  ██████╗████████╗██╗     
 * ██║██╔═══██╗██╔════╝╚══██╔══╝██║     
 * ██║██║   ██║██║        ██║   ██║     
 * ██║██║   ██║██║        ██║   ██║     
 * ██║╚██████╔╝╚██████╗   ██║   ███████╗
 * ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
 *
 ***********************************************************************************/
#ifdef CONFIG_COMPAT
long pulp_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  return pulp_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

long pulp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  int err = 0, i, j;
  long retval = 0;
  
  // to read from user space
  unsigned request[3];
  unsigned long n_bytes_read, n_bytes_left;
  unsigned byte;

  // what we get from user space
  unsigned size_b;
   
  // what get_user_pages needs
  struct page ** pages;
  unsigned       len; 
  
  // what mem_map_sg needs needs
  unsigned *      addr_start_vec;
  unsigned *      addr_end_vec;
  unsigned long * addr_offset_vec;
  unsigned *      page_idxs_start;
  unsigned *      page_idxs_end;
  unsigned        n_segments;

  // needed for cache flushing
  unsigned offset_start, offset_end;

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
  if ( (_IOC_NR(cmd) < 0xB0) | (_IOC_NR(cmd) > 0xB7) ) return -ENOTTY;

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

  // the actual ioctls {{{
  switch(cmd) {

  case PULP_IOCTL_RAB_REQ: // request new RAB slices

    retval = pulp_rab_req(my_dev.rab_config, arg);

    break;

  case PULP_IOCTL_RAB_FREE: // free RAB slices based on time code

    pulp_rab_free(my_dev.rab_config, arg);

    #if PLATFORM == JUNO
      pulp_rab_ax_log_read(pulp_cluster_select, 1);
      pulp_rab_ax_log_print();
    #endif

    #if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
      pulp_rab_prof_print();
    #endif

    break;
    
  case PULP_IOCTL_RAB_REQ_STRIPED: // request striped RAB slices

    retval = pulp_rab_req_striped(my_dev.rab_config, arg);

    break;

  case PULP_IOCTL_RAB_FREE_STRIPED: // free striped RAB slices

    pulp_rab_free_striped(my_dev.rab_config, arg);

    #if PLATFORM == JUNO
      pulp_rab_ax_log_read(pulp_cluster_select, 1);
      pulp_rab_ax_log_print();
    #endif

    #if defined(PROFILE_RAB_STR) || defined(PROFILE_RAB_MH)
      pulp_rab_prof_print();
    #endif

    break;

  // DMAC_XFER {{{
  case PULP_IOCTL_DMAC_XFER: // setup a transfer using the PL330 DMAC inside Zynq
  
    // get transfer data from user space - arg already checked above
    byte = 0;
    n_bytes_left = 3*sizeof(unsigned);
    n_bytes_read = n_bytes_left;
    while (n_bytes_read > 0) {
      n_bytes_left = __copy_from_user((void *)((char *)request+byte),
                                      (void __user *)((char *)arg+byte), n_bytes_read);
      byte += (n_bytes_read - n_bytes_left);
      n_bytes_read = n_bytes_left;
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
    err = pulp_mem_get_user_pages(&pages, addr_l3, len, dma_cmd);
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
  // }}}

  // RAB_MH_ENA {{{
  case PULP_IOCTL_RAB_MH_ENA:

    // get slice data from user space - arg already checked above
    byte = 0;
    n_bytes_left = 2*sizeof(unsigned); 
    n_bytes_read = n_bytes_left;
    while (n_bytes_read > 0) {
      n_bytes_left = __copy_from_user((void *)((char *)request+byte),
                             (void __user *)((char *)arg+byte), n_bytes_read);
      byte += (n_bytes_read - n_bytes_left);
      n_bytes_read = n_bytes_left;
    }
    rab_mh_acp = request[0];
    rab_mh_lvl = request[1];

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
    for (i=0; i<N_CORES_TIMER; i++) {
      clk_cntr_refill[i] = 0;
      clk_cntr_sched[i] = 0;
      clk_cntr_resp[i] = 0;
    }
    n_misses = 0;
    n_first_misses = 0;
    clk_cntr_setup = 0;
    clk_cntr_cache_flush = 0;
    clk_cntr_get_user_pages = 0;
    iowrite32(0, (void *)((unsigned long)my_dev.l3_mem+N_MISSES_OFFSET_B));
#endif

    // enable - protect with semaphore?
    rab_mh = 1;
    rab_mh_date = 1;

    printk(KERN_INFO "PULP: RAB miss handling enabled.\n");

    break;
  // }}}

  // RAB_MH_DIS {{{
  case PULP_IOCTL_RAB_MH_DIS:
   
    // disable - protect with semaphore?
    rab_mh = 0;
    rab_mh_date = 0;

#ifdef PROFILE_RAB_MH
    for (i=0; i<N_CORES_TIMER; i++) {
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
  // }}}

  case PULP_IOCTL_INFO_PASS: // pass info from user to kernel space

    // get gpio value from slice data from user space - arg already checked above
    byte = 0;
    n_bytes_left = 1*sizeof(unsigned); 
    n_bytes_read = n_bytes_left;    
    while (n_bytes_read > 0) {
      n_bytes_left = __copy_from_user((void *)((char *)request+byte),
                             (void __user *)((char *)arg+byte), n_bytes_read);
      byte += (n_bytes_read - n_bytes_left);
      n_bytes_read = n_bytes_left;
    }

    pulp_cluster_select = request[0];

    break;

  default:
    return -ENOTTY;
  }
  // }}}

  return retval;
}
// }}}

// rab_handle_miss {{{
/***********************************************************************************
 *
 * ███╗   ███╗██╗  ██╗██████╗ 
 * ████╗ ████║██║  ██║██╔══██╗
 * ██╔████╔██║███████║██████╔╝
 * ██║╚██╔╝██║██╔══██║██╔══██╗
 * ██║ ╚═╝ ██║██║  ██║██║  ██║
 * ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
 *
 ***********************************************************************************/
static void pulp_rab_handle_miss(unsigned unused)
{
  int err, i, j, handled, result;
  unsigned id, id_pe, id_cluster;
  unsigned addr_start;
  unsigned long addr_phys;
    
  // what get_user_pages needs
  unsigned long start;
  struct page **pages;
  unsigned write;

  // needed for RAB management
  RabSliceReq rab_slice_request;
  RabSliceReq *rab_slice_req = &rab_slice_request;

  // needed for L2 TLB
  L2Entry l2_entry_request;
  L2Entry *l2_entry = &l2_entry_request;
  int use_l1 = 1;
  int err_l2 = 0;

  if (DEBUG_LEVEL_RAB_MH > 1)
    printk(KERN_INFO "PULP: RAB miss handling routine started.\n");
  
  // empty miss-handling FIFOs
  for (i=0; i<RAB_MH_FIFO_DEPTH; i++) {
    rab_mh_addr[i] = IOREAD_L((void *)((unsigned long)my_dev.rab_config+RAB_MH_ADDR_FIFO_OFFSET_B));
    rab_mh_id[i]   = IOREAD_L((void *)((unsigned long)my_dev.rab_config+RAB_MH_ID_FIFO_OFFSET_B));
    
    // detect empty FIFOs
#if PLATFORM != JUNO    
    if ( rab_mh_id[i] & 0x80000000 )
#else
    if ( rab_mh_id[i] & 0x8000000000000000 )
#endif      
      break;
    if (DEBUG_LEVEL_RAB_MH > 0) {
      printk(KERN_INFO "PULP: RAB miss - i = %d, date = %#x, id = %#x, addr = %#x\n",
       i, rab_mh_date, rab_mh_id[i], rab_mh_addr[i]);
    }
     
    // handle every miss separately
    err = 0;
    addr_start = rab_mh_addr[i];
    id = rab_mh_id[i];
    
    // check the ID
    id_pe      = BF_GET(id, 0, AXI_ID_WIDTH_CORE);
    id_cluster = BF_GET(id, AXI_ID_WIDTH_CLUSTER + AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_SOC);
    // - 0x3;
    //id_cluster = BF_GET(id, AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_CLUSTER) - 0x2;

    // identify RAB port - for now, only handle misses on Port 1: PULP -> Host
    if ( BF_GET(id, AXI_ID_WIDTH, 1) )
      rab_slice_req->rab_port = 1;    
    else {
      printk(KERN_WARNING "PULP: Cannot handle RAB miss on ports different from Port 1.\n");
      printk(KERN_WARNING "PULP: RAB miss - id = %#x, addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);
      continue;
    }

    // only handle misses from PEs' data interfaces
    if ( ( BF_GET(id, AXI_ID_WIDTH_CORE, AXI_ID_WIDTH_CLUSTER) != 0x2 ) ||
         ( (id_cluster < 0) || (id_cluster > (N_CLUSTERS-1)) ) || 
         ( (id_pe < 0) || (id_pe > N_CORES-1) ) ) {
      printk(KERN_WARNING "PULP: Can only handle RAB misses originating from PE's data interfaces. id = %#x | addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);
      // for debugging
      //      pulp_rab_mapping_print(my_dev.rab_config,0xAAAA);

      continue;
    }
  
#ifdef PROFILE_RAB_MH
    err = ioread32((void *)((unsigned long)my_dev.l3_mem+N_MISSES_OFFSET_B));
    // reset internal counters
    if (!err) {      
      clk_cntr_setup = 0;
      clk_cntr_cache_flush = 0;
      clk_cntr_get_user_pages = 0;
      n_first_misses = 0;
      n_misses = 0;
      for (j=0; j<N_CORES_TIMER; j++) {
        clk_cntr_refill[j] = 0;
        clk_cntr_sched[j]  = 0;
        clk_cntr_resp[j]   = 0;
      }
    }
    n_misses++;

    // read the PE timer
    clk_cntr_sched[id_pe] += ioread32((void *)((unsigned long)my_dev.clusters
                                               +TIMER_GET_TIME_LO_OFFSET_B+(id_pe+1)*PE_TIMER_OFFSET_B));
    // save resp_tmp
    clk_cntr_resp[id_pe] += clk_cntr_resp_tmp[id_pe];
#endif

    // check if there has been a miss to the same page before
    handled = 0;    
    for (j=0; j<i; j++) {
      if ( (addr_start & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned)*8-PAGE_SHIFT))
           == (rab_mh_addr[j] & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned)*8-PAGE_SHIFT)) ) {
        handled = 1;
        if (DEBUG_LEVEL_RAB_MH > 0) {
          printk(KERN_WARNING "PULP: Already handled a miss to this page.\n");
          // for debugging only - deactivate fetch enable
          // iowrite32(0xC0000000,(void *)((unsigned long)my_dev.gpio+0x8));
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
      if ( pages == NULL ) {
        printk(KERN_WARNING "PULP: Memory allocation failed.\n");
        err = 1;
        goto miss_handling_error;
      }

      // align address to page border / 4kB
      start = (unsigned long)(addr_start) & BF_MASK_GEN(PAGE_SHIFT,sizeof(unsigned long)*8-PAGE_SHIFT);
      
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
      addr_phys = (unsigned long)page_to_phys(pages[0]);
      if (DEBUG_LEVEL_MEM > 1) {
        printk(KERN_INFO "PULP: Physical address = %#lx\n",addr_phys);
      }

#ifdef PROFILE_RAB_MH
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_get_user_pages += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif
      
      // fill rab_slice_req structure
      // allocate a fixed number of slices to each PE???
      // works for one port only!!!
      rab_slice_req->date_cur = (unsigned char)rab_mh_date;
      rab_slice_req->date_exp = (unsigned char)rab_mh_date; 
      rab_slice_req->page_idx_start = 0;
      rab_slice_req->page_idx_end   = 0;
      rab_slice_req->rab_mapping = (unsigned)pulp_rab_mapping_get_active();
      rab_slice_req->flags_drv = 0;

      rab_slice_req->addr_start  = (unsigned)start;
      rab_slice_req->addr_end    = (unsigned)start + PAGE_SIZE;
      rab_slice_req->addr_offset = addr_phys;
      RAB_SET_PROT(rab_slice_req->flags_hw,(write << 2) | 0x2 | 0x1);
      RAB_SET_ACP(rab_slice_req->flags_hw,rab_mh_acp);

      // Check which TLB level to use.
      // If rab_mh_lvl is 0, enter the entry in L1 if free slice is available. If not, enter in L2.
      if (rab_mh_lvl == 2) {
        use_l1 = 0;
      } else {
        // get a free slice
        use_l1 = 1;
        err = pulp_rab_slice_get(rab_slice_req);
        if (err) {
          if (rab_mh_lvl == 1) {
            printk(KERN_WARNING "PULP: RAB miss handling error 2.\n");
            goto miss_handling_error;
          }
          else
            use_l1 = 0;
        }
      }
      err = 0;
      if (use_l1 == 0) { // Use L2
        l2_entry->flags = rab_slice_req->flags_hw;
        l2_entry->pfn_v = (unsigned)(start >> PAGE_SHIFT);
        l2_entry->pfn_p = (unsigned)(addr_phys >> PAGE_SHIFT);
        l2_entry->page_ptr = pages[0];
        err_l2 = pulp_rab_l2_setup_entry(my_dev.rab_config, l2_entry, rab_slice_req->rab_port, 1);   
      } else { // Use L1
        // free memory of slices to be re-configured
        pulp_rab_slice_free(my_dev.rab_config, rab_slice_req);

        // check for free field in page_ptrs list
        err = pulp_rab_page_ptrs_get_field(rab_slice_req);
        if (err) {
          printk(KERN_WARNING "PULP: RAB miss handling error 0.\n");
          goto miss_handling_error;
        }
                
        // setup slice
        err = pulp_rab_slice_setup(my_dev.rab_config, rab_slice_req, pages);
        if (err) {
          printk(KERN_WARNING "PULP: RAB miss handling error 1.\n");
          goto miss_handling_error;
        }        
      } // if (use_l1 == 0)

#ifdef PROFILE_RAB_MH
      arm_clk_cntr_value_start = arm_clk_cntr_value;

      // read the ARM clock counter
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_cntr_value) : );
      clk_cntr_setup += (arm_clk_cntr_value - arm_clk_cntr_value_start);
#endif

      // flush the entire page from the caches if ACP is not used
      if (!rab_mh_acp)
        pulp_mem_cache_flush(pages[0],0,PAGE_SIZE);

      if (use_l1 == 0)
        kfree(pages);

      if (rab_mh_lvl == 1)
        if (rab_slice_req->rab_slice == (RAB_L1_N_SLICES_PORT_1 - 1)) {
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
    // pulp_rab_mapping_print(my_dev.rab_config,0xAAAA);

    // wake up the sleeping PE
    iowrite32(BIT_N_SET(id_pe),
              (void *)((unsigned long)my_dev.clusters + CLUSTER_SIZE_B*id_cluster
                       + CLUSTER_PERIPHERALS_OFFSET_B + BBMUX_CLKGATE_OFFSET_B + GP_2_OFFSET_B));

    /*
     * printk(KERN_INFO "WAKEUP: value = %#x, address = %#x\n",BIT_N_SET(id_pe), CLUSTER_SIZE_B*id_cluster \
     *        + CLUSTER_PERIPHERALS_OFFSET_B + BBMUX_CLKGATE_OFFSET_B + GP_2_OFFSET_B );
     */

#ifdef PROFILE_RAB_MH
    // read the PE timer
    clk_cntr_refill[id_pe] += ioread32((void *)((unsigned long)my_dev.clusters
                                                +TIMER_GET_TIME_LO_OFFSET_B+(id_pe+1)*PE_TIMER_OFFSET_B));
    // update counters in shared memory
    iowrite32(n_misses, (void *)((unsigned long)my_dev.l3_mem+N_MISSES_OFFSET_B));
#endif
    // error handling
  miss_handling_error:
    if (err) {
      printk(KERN_WARNING "PULP: Cannot handle RAB miss: id = %#x, addr = %#x\n", rab_mh_id[i], rab_mh_addr[i]);

      // for debugging
      pulp_rab_mapping_print(my_dev.rab_config,0xAAAA);
    }

  }
  if (DEBUG_LEVEL_RAB_MH > 1)
    printk(KERN_INFO "PULP: RAB miss handling routine finished.\n");

 
#ifdef PROFILE_RAB_MH
  // update counters in shared memory
  iowrite32(n_first_misses,
            (void *)((unsigned long)my_dev.l3_mem+N_FIRST_MISSES_OFFSET_B));
  iowrite32(clk_cntr_setup,
            (void *)((unsigned long)my_dev.l3_mem+CLK_CNTR_SETUP_OFFSET_B));
  iowrite32(clk_cntr_cache_flush,
            (void *)((unsigned long)my_dev.l3_mem+CLK_CNTR_CACHE_FLUSH_OFFSET_B));
  iowrite32(clk_cntr_get_user_pages,
            (void *)((unsigned long)my_dev.l3_mem+CLK_CNTR_GET_USER_PAGES_OFFSET_B));

  for (j=0; j<N_CORES_TIMER; j++) {
    iowrite32(clk_cntr_refill[j],
              (void *)((unsigned long)my_dev.l3_mem+CLK_CNTR_REFILL_OFFSET_B+4*j));
    iowrite32(clk_cntr_sched[j],
              (void *)((unsigned long)my_dev.l3_mem+CLK_CNTR_SCHED_OFFSET_B+4*j));
    iowrite32(clk_cntr_resp[j],
              (void *)((unsigned long)my_dev.l3_mem+CLK_CNTR_RESP_OFFSET_B+4*j));
  }

#endif
}
// }}}

// vim: ts=2 sw=2 sts=2 et foldmethod=marker tw=100
