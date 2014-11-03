#include "pulp_func.h"
#include "pulp_host.h"

//#include <errno.h>
//printf("ERRNO = %i\n",errno);


/*
 * Reserve the virtual address space overlapping with the physical
 * address map of pulp using the mmap() syscall with MAP_FIXED and
 * MAP_ANONYMOUS
 */
int pulp_reserve_v_addr(PulpDev *pulp) {

  pulp->reserved_v_addr.size = PULP_SIZE_B;
  pulp->reserved_v_addr.v_addr = mmap((int *)PULP_BASE_ADDR,pulp->reserved_v_addr.size,PROT_NONE,MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,-1,0);
  if (pulp->reserved_v_addr.v_addr == MAP_FAILED) {
    printf("MMAP failed to reserve virtual addresses overlapping with physical address map of PULP.\n");
    return EIO;
  }
  else {
    printf("Reserved virtual addresses starting at %p and overlapping with physical address map of PULP. \n",pulp->reserved_v_addr.v_addr);
  }
  
  return 0;
}

/*
 * Free the virtual address space overlapping with the physical
 * address map of pulp using the munmap() syscall
 */
int pulp_free_v_addr(PulpDev *pulp) {
  
  int status;

  printf("Freeing reserved virtual addresses to overlapping with physical address map of PULP.\n");

  status = munmap(pulp->reserved_v_addr.v_addr,pulp->reserved_v_addr.size);
  if (status) {
    printf("MUNMAP failed to free reserved virtual addresses overlapping with physical address map of PULP.\n");
  } 

  return 0;
}

/*
 * Read 32 bits
 * off: offset
 * off_type: type of the offset, 'b' = byte offset, else word offset
 */
int pulp_read32(unsigned *base_addr, unsigned off, char off_type) {
  if (DEBUG_LEVEL > 3) {
    unsigned *addr;
    if (off_type == 'b') 
      addr = base_addr + (off>>2);
    else
      addr = base_addr + off;
    printf("Reading from %p\n",addr);
  }
  if (off_type == 'b')
    return *(base_addr + (off>>2));
  else
    return *(base_addr + off);
}

/*
 * Write 32 bits
 * off: offset
 * off_type: type of the offset, 'b' = byte offset, else word offset
 */
void pulp_write32(unsigned *base_addr, unsigned off, char off_type, unsigned value) {
  if (DEBUG_LEVEL > 3) {
    unsigned *addr;
    if (off_type == 'b') 
      addr = base_addr + (off>>2);
    else
      addr = base_addr + off;
    printf("Writing to %p\n",addr);
  }
  if (off_type == 'b')
    *(base_addr + (off>>2)) = value;
  else
    *(base_addr + off) = value;
}

/*
 * Setup a DMA transfer
 * pulp: pointer to the PulpDev structure
 * source_addr: source address
 * dest_addr: destination address
 * length_b: length of the DMA transfer in bytes
 * dma: DMA engine to select
 */
int pulp_dma_transfer(PulpDev *pulp, unsigned source_addr, unsigned dest_addr, unsigned length_b, unsigned dma) {
 
  // unsigned *dma_addr;
  // 
  // /*
  //  * Select the DMA engine based on source and destination addresses
  //  */
  // dma_addr = pulp->clusters.v_addr + (CLUSTER_DMA_OFFSET_B >> 2);
  // //printf("DMA command FIFO @ %p.\n",dma_addr);
  // 
  // int status,dma_status,n_trials;
  // status = 0;
  // n_trials = 10000;
  // while (!status && n_trials) {
  //   //printf("DMA status: %#x.\n",pulp_read32(pulp->clusters.v_addr,CLUSTER_DMA_OFFSET_B+0xC,'b'));
  //   dma_status = pulp_read32(dma_addr,0xC,'b');
  //   if (dma_status == 0) {
  //     status = 1;
  //     printf("DMA engine ready, initialize transfer.\n");
  //   }
  //   else {
  //     if (DEBUG_LEVEL > 3) {
  // 	printf("DMA status = %#x\n",dma_status);
  //     }
  //     n_trials--;
  //     usleep(100);
  //   }
  // }
  // if (!status) {
  //   printf("Command FIFO of DMA engine full or DMA engine busy. Transfer dropped.\n");
  //   return EBUSY;
  // } 
  // 
  // /*
  //  * Init transfer
  //  */
  // pulp_write32(dma_addr,0x4,'b',source_addr);
  // pulp_write32(dma_addr,0x4,'b',dest_addr);
  // pulp_write32(dma_addr,0x4,'b',length_b);

 // unsigned *dma_base_addr;
 // dma_base_addr = pulp->soc_periph.v_addr + (CDMA_SIZE_B*dma >> 2);
 // 
 // /*
 //  * Check availability
 //  */
 // unsigned status, n_trials;
 // unsigned dma_status;  
 // status = 0;
 // n_trials = 1000;
 //
 // while (!status && n_trials) {
 //   dma_status = pulp_read32(dma_base_addr,0x4,'b');
 //   if (dma_status & 0x2) { // idle bit set
 //     if (dma_status & 0x1000) { // interrupt on complete set
 //	pulp_write32(dma_base_addr,0x4,'b',0x1000); // write interrupt bit to reset it to 0
 //     }
 //     status = 1;
 //     if (DEBUG_LEVEL > 2)
 //	printf("DMA engine %i ready, initialize transfer.\n",dma);
 //   }
 //   else {
 //     if (DEBUG_LEVEL > 2)
 //	printf("DMA status = %#x\n",dma_status);
 //     n_trials--;
 //     usleep(1);
 //   }
 // }
 // if (!status) {
 //   printf("DMA engine %i busy. Transfer dropped.\n",dma);
 //   return EBUSY; 
 // }
 //     
 // /*
 //  * Init transfer
 //  */
 // pulp_write32(dma_base_addr,0x18,'b',source_addr);
 // pulp_write32(dma_base_addr,0x20,'b',dest_addr);
 // pulp_write32(dma_base_addr,0x28,'b',length_b);
 
  return 0;
}

/*
 * Wait until the specified DMA engine is idle
 * pulp: pointer to the PulpDev structure
 * dma: DMA engine to wait for
 */
int pulp_dma_wait(PulpDev *pulp, unsigned dma) {
 // unsigned *dma_base_addr;
 // dma_base_addr = pulp->soc_periph.v_addr + (CDMA_SIZE_B*dma >> 2);
 // 
 // unsigned status, n_trials;
 // unsigned dma_status;
 // status = 0;
 // n_trials = 10000;
 // 
 // while (!status && n_trials) {
 //   dma_status = pulp_read32(dma_base_addr,0x4,'b');
 //   if (DEBUG_LEVEL > 2) 
 //     printf("DMA status = %#x\n",dma_status);
 //   if ((dma_status & 0x1000) && (dma_status & 0x2)) { // interrupt on complete set, idle
 //     status = 1;
 //     // reset the interrupt on complete
 //     pulp_write32(dma_base_addr,0x4,'b',0x1000);
 //     if (DEBUG_LEVEL > 2)
 // 	printf("DMA engine %i ready.\n",dma);
 //   }
 //   n_trials--;
 //   usleep(100);
 // }
 // if (!status) {
 //   printf("DMA engine %i busy - timeout.\n",dma);
 //   return EBUSY;
 // }

  return 0;
}

/*
 * Reset the specified DMA engine
 * pulp: pointer to the PulpDev structure
 * dma: DMA engine to reset
 */
int pulp_dma_reset(PulpDev *pulp, unsigned dma) {
  // unsigned *dma_base_addr;
  // dma_base_addr = pulp->soc_periph.v_addr + (CDMA_SIZE_B*dma >> 2);
  // 
  // // set the reset
  // pulp_write32(dma_base_addr,0,'b',0x4);
  // usleep(10);
  // // release
  // pulp_write32(dma_base_addr,0,'b',0x0);
  // 
  // if (DEBUG_LEVEL > 2) {
  //   printf("DMA engine %i has been reset.\n",dma);
  // }
 
  return 0;
}

/*
 * Memory map the device to virtual user space using the mmap()
 * syscall
 */
int pulp_mmap(PulpDev *pulp) {

  int offset;

  /*
   *  open the device
   */
  pulp->fd = open("/dev/PULP", O_RDWR | O_SYNC);
  if (pulp->fd < 0) {
    printf("Opening failed \n");
    return ENOENT;
  }

  /*
   *  do the different remappings
   */
  // L3
  offset = 0; // place wheres the OpenRISC stores the results in L3
  pulp->l3_mem.size = L3_MEM_SIZE_B;
    
  pulp->l3_mem.v_addr = mmap(NULL,pulp->l3_mem.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->l3_mem.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("Shared L3 memory mapped to virtual user space at %p.\n",pulp->l3_mem.v_addr);
  }
  
  // Clusters
  offset = L3_MEM_SIZE_B; // start of clusters
  pulp->clusters.size = CLUSTERS_SIZE_B;
  
  pulp->clusters.v_addr = mmap(NULL,pulp->clusters.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->clusters.v_addr == MAP_FAILED) {
    printf("MMAP failed for clusters.\n");
    return EIO;
  }
  else {
     printf("Clusters mapped to virtual user space at %p.\n",pulp->clusters.v_addr);
  }

  // SOC_PERIPHERALS
  offset = L3_MEM_SIZE_B + CLUSTERS_SIZE_B; // start of peripherals
  pulp->soc_periph.size = SOC_PERIPHERALS_SIZE_B;
 
  pulp->soc_periph.v_addr = mmap(NULL,pulp->soc_periph.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->soc_periph.v_addr == MAP_FAILED) {
    printf("MMAP failed for SoC peripherals.\n");
    return EIO;
  }
  else {
    printf("SoC peripherals mapped to virtual user space at %p.\n",pulp->soc_periph.v_addr);
  }

  // L2
  offset = L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B; // start of L2
  pulp->l2_mem.size = L2_MEM_SIZE_B;
 
  pulp->l2_mem.v_addr = mmap(NULL,pulp->l2_mem.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
 
  if (pulp->l2_mem.v_addr == MAP_FAILED) {
    printf("MMAP failed for L2 memory.\n");
    return EIO;
  }
  else {
     printf("L2 memory mapped to virtual user space at %p.\n",pulp->l2_mem.v_addr);
  }
  
  // RAB
  offset = L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B; // start of RAB config
  pulp->rab_config.size = RAB_CONFIG_SIZE_B;
 
  pulp->rab_config.v_addr = mmap(NULL,pulp->rab_config.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->rab_config.v_addr == MAP_FAILED) {
    printf("MMAP failed for RAB config.\n");
    return EIO;
  }
  else {
    printf("RAB config memory mapped to virtual user space at %p.\n",pulp->rab_config.v_addr);
  }

  // GPIO
  offset = L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B + RAB_CONFIG_SIZE_B; // start of GPIOroot

  pulp->gpio.size = H_GPIO_SIZE_B;
 
  pulp->gpio.v_addr = mmap(NULL,pulp->gpio.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->gpio.v_addr == MAP_FAILED) {
    printf("MMAP failed for GPIO.\n");
    return EIO;
  }
  else {
    printf("GPIO memory mapped to virtual user space at %p.\n",pulp->gpio.v_addr);
  }

  // SLCR
  offset = L3_MEM_SIZE_B + CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + L2_MEM_SIZE_B + RAB_CONFIG_SIZE_B + H_GPIO_SIZE_B; // start of slcr
  pulp->slcr.size = SLCR_SIZE_B;
 
  pulp->slcr.v_addr = mmap(NULL,pulp->slcr.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->slcr.v_addr == MAP_FAILED) {
    printf("MMAP failed for SLCR.\n");
    return EIO;
  }
  else {
    printf("SLCR mapped to virtual user space at %p.\n",pulp->slcr.v_addr);
  }
  
  return 0;
}

/*
 * Undo the memory mapping of the device to virtual user space using
 * the munmap() syscall
 */
int pulp_munmap(PulpDev *pulp) {

  unsigned status;

  // undo the memory mappings
  printf("Undo the memory mappings.\n");
  status = munmap(pulp->slcr.v_addr,pulp->slcr.size);
  if (status) {
    printf("MUNMAP failed for SLCR.\n");
  } 
  status = munmap(pulp->gpio.v_addr,pulp->gpio.size);
  if (status) {
    printf("MUNMAP failed for GPIO.\n");
  } 
  status = munmap(pulp->rab_config.v_addr,pulp->rab_config.size);
  if (status) {
    printf("MUNMAP failed for RAB config.\n");
  } 
  status = munmap(pulp->l2_mem.v_addr,pulp->l2_mem.size);
  if (status) {
    printf("MUNMAP failed for L2 memory.\n");
  } 
  status = munmap(pulp->soc_periph.v_addr,pulp->soc_periph.size);
  if (status) {
    printf("MUNMAP failed for SoC peripherals\n.");
  }
  status = munmap(pulp->clusters.v_addr,pulp->clusters.size);
  if (status) {
    printf("MUNMAP failed for clusters\n.");
  }
  status = munmap(pulp->l3_mem.v_addr,pulp->l3_mem.size);
  if (status) {
    printf("MUNMAP failed for shared L3 memory\n.");
  }
   
  // close the file descriptor
  printf("Close the file descriptor. \n");
  close(pulp->fd);

  return 0;
}

/*
 * Initialize the memory mapped device 
 */
int pulp_init(PulpDev *pulp) {

  unsigned offset;

  // init sequence
  // release PL reset
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0);
  usleep(10);
  pulp_write32(pulp->gpio.v_addr,0x8,'b',3);

  // // keep cores in reset, set fetch enable to 0
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0xc,'b',0);
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x4,'b',0);
  
  // // initialize the demux (core id, cluster id and global cluster offset)
  // pulp_write32(pulp->demux_config.v_addr,0x0,'b',0);
  // pulp_write32(pulp->demux_config.v_addr,0x4,'b',0);
  // pulp_write32(pulp->demux_config.v_addr,0x8,'b',PULP_CLUSTER_OFFSET);
  
  // setup the cluster DMA
  //pulp_write32(pulp->clusters.v_addr,CLUSTER_DMA_OFFSET_B+0x10,'b',PULP_CLUSTER_OFFSET);

  // setup the RAB
  // port 0: Host -> PULP
  // L2 memory
  offset = 0x10*((RAB_N_PORTS-3)*RAB_N_SLICES+0);
  printf("RAB config offset = %#x\n",offset);
  pulp_write32(pulp->rab_config.v_addr,offset+0x10,'b',PULP_H_BASE_ADDR);
  pulp_write32(pulp->rab_config.v_addr,offset+0x14,'b',PULP_H_BASE_ADDR+L2_MEM_SIZE_B);
  pulp_write32(pulp->rab_config.v_addr,offset+0x18,'b',L2_MEM_BASE_ADDR);
  pulp_write32(pulp->rab_config.v_addr,offset+0x1c,'b',0x7);
  
  // port 1: PULP -> Host
  // L3 memory (contiguous)
  offset = 0x10*((RAB_N_PORTS-2)*RAB_N_SLICES+0);
  printf("RAB config offset = %#x\n",offset);
  pulp_write32(pulp->rab_config.v_addr,offset+0x10,'b',L3_MEM_BASE_ADDR);
  pulp_write32(pulp->rab_config.v_addr,offset+0x14,'b',L3_MEM_BASE_ADDR+L3_MEM_SIZE_B);
  pulp_write32(pulp->rab_config.v_addr,offset+0x18,'b',L3_MEM_BASE_ADDR);
  pulp_write32(pulp->rab_config.v_addr,offset+0x1c,'b',0x7);

  // // enable mailbox interrupts
  // pulp_write32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_IE_OFFSET_B,'b',0x6);
  // pulp_write32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_IE_OFFSET_B,'b',0x4);

  return 0;
}

int pulp_offload_kernel(PulpDev *pulp, bool sync) {
   
  char kernel[10];
  strcpy(kernel,"value.txt");
  
  // open the file
  FILE * code_file;
  code_file = fopen(kernel,"r");
  if (code_file == NULL) {
    printf("ERROR, could not open code file.\n");
    return ENOENT;
  }

  // offload code to the OR
  unsigned int buffer, offset;
  offset = 0;
  while (!feof(code_file)) {
    fscanf(code_file, "%x", &buffer);
    pulp_write32(pulp->l3_mem.v_addr,offset,'w',buffer);
    offset++;
  }
  fclose(code_file);
 
  // // test_interrupts
  // // clear L2
  // pulp_write32(pulp->l2_mem.v_addr,0,'b',0);
  // pulp_write32(pulp->l2_mem.v_addr,0x4,'b',0);
  //  
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface1 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // 
  // // write to mailbox
  // pulp_write32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_WRDATA_OFFSET_B,'b',0xFF);
  // 
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface1 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // 
  // // start execution
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x10,'b',BOOT_ADDR);
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0xc,'b',1);
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x4,'b',1);
  
  // wait for PULP
  sleep(1);
  
  // stop execution
  //pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0xc,'b',0);
  //pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x4,'b',0);

  // test_mailbox
  // // clear L2
  // pulp_write32(pulp->l2_mem.v_addr,0,'b',0);
  // pulp_write32(pulp->l2_mem.v_addr,0x4,'b',0);
  // pulp_write32(pulp->l2_mem.v_addr,0x8,'b',0);
  // 
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface1 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // 
  // // write to mailbox
  // pulp_write32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_WRDATA_OFFSET_B,'b',0xFF);
  // 
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface1 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // 
  // // start execution
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x10,'b',BOOT_ADDR);
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0xc,'b',1);
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x4,'b',1);
  // 
  // // wait for PULP
  // sleep(1);
  // 
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface1 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // 
  // // read status variable in L2
  // printf("Read status variable in L2. Is equal 2 if PULP is polling mailbox.\n");
  // printf("Status variable in L2: %#x \n", pulp_read32(pulp->l2_mem.v_addr,0,'b'));
  // 
  // // write to mailbox
  // printf("Write 0xAA to mailbox Interface0.\n");
  // pulp_write32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_WRDATA_OFFSET_B,'b',0xAA);
  // 
  // // wait for PULP
  // sleep(1);
  // 
  // // stop execution
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0xc,'b',0);
  // pulp_write32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x4,'b',0);

  return 0;
}

int pulp_check_results(PulpDev *pulp) {

  // check results
  int test;
  //test = pulp_read32(pulp->clusters.v_addr,CLUSTER_CONTROLLER_OFFSET_B+0x4,'b');
  printf("Fetch enable = %d.\n",test);
  if (test == 1) {
    sleep(1);
  }

  // // test_interrupts
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface1 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // 
  // // read from mailbox
  // printf("PULP should have written 0xFFFF to its write interface.\n");
  // printf("Read from mailbox Interface 0: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_RDDATA_OFFSET_B,'b'));
  // 
  // // read status variable in L2
  // printf("Read status variable in L2. Should be equal 2.\n");
  // printf("Status variable in L2: %#x \n", pulp_read32(pulp->l2_mem.v_addr,0,'b'));
  // printf("Data variable in L2: %#x \n", pulp_read32(pulp->l2_mem.v_addr,0x4,'b'));

  // // test_mailbox
  // // read mailbox status
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE0_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  // printf("Mailbox Interface0 status: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_STATUS_OFFSET_B,'b'));
  //
  // // read from mailbox
  // printf("PULP should have written 0xAA to its write interface.\n");
  // printf("Read from mailbox Interface1: %#x \n", pulp_read32(pulp->clusters.v_addr,MAILBOX_INTERFACE1_OFFSET_B+MAILBOX_RDDATA_OFFSET_B,'b'));

  //// L2
  //printf("L2 - 1: %#x \n",pulp_read32(pulp->l2_mem.v_addr,0,'w'));
  //printf("L2 - 2: %#x \n",pulp_read32(pulp->l2_mem.v_addr,1,'w'));
  //printf("L2 - 3: %#x \n",pulp_read32(pulp->l2_mem.v_addr,2,'w'));
  //
  //// TCDM
  //printf("TCDM - 1: %#x \n",pulp_read32(pulp->clusters.v_addr,0x5000,'b'));
  //printf("TCDM - 2: %#x \n",pulp_read32(pulp->clusters.v_addr,0x5004,'b'));
  //printf("TCDM - 3: %#x \n",pulp_read32(pulp->clusters.v_addr,0x5008,'b'));
  //
  //// L3
  //printf("L3 - 1: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x10000,'b'));
  //printf("L3 - 2: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x10004,'b'));
  //printf("L3 - 3: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x10008,'b'));

  return 0;
};


void pulp_reset(PulpDev *pulp) {

  unsigned slcr_value;

  // FPGA reset control register
  slcr_value = pulp_read32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b');
  
  // Extract the FPGA_OUT_RST bits
  slcr_value = slcr_value & 0xF; ;

  // Enable reset
  pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', slcr_value | (0x1 << SLCR_FPGA_OUT_RST));

  // Wait
  usleep(10);
  
  // Disable reset
  pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', slcr_value & (0xF & (0x0 << SLCR_FPGA_OUT_RST)));

}
