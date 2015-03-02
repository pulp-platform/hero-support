#include "pulp_func.h"
#include "pulp_host.h"

/**
 * Reserve the virtual address space overlapping with the physical
 * address map of pulp using the mmap() syscall with MAP_FIXED and
 * MAP_ANONYMOUS
 * @pulp: pointer to the PulpDev structure
 */
int pulp_reserve_v_addr(PulpDev *pulp)
{
  pulp->reserved_v_addr.size = PULP_SIZE_B;
  pulp->reserved_v_addr.v_addr = mmap((int *)PULP_BASE_ADDR,pulp->reserved_v_addr.size,
				      PROT_NONE,MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,-1,0);
  if (pulp->reserved_v_addr.v_addr == MAP_FAILED) {
    printf("MMAP failed to reserve virtual addresses overlapping with physical address map of PULP.\n");
    return EIO;
  }
  else {
    printf("Reserved virtual addresses starting at %p and overlapping with physical address map of PULP. \n",
	   pulp->reserved_v_addr.v_addr);
  }
  
  return 0;
}

/**
 * Free the virtual address space overlapping with the physical
 * address map of pulp using the munmap() syscall
 * @pulp: pointer to the PulpDev structure
 */
int pulp_free_v_addr(PulpDev *pulp)
{  
  int status;

  printf("Freeing reserved virtual addresses overlapping with physical address map of PULP.\n");

  status = munmap(pulp->reserved_v_addr.v_addr,pulp->reserved_v_addr.size);
  if (status) {
    printf("MUNMAP failed to free reserved virtual addresses overlapping with physical address map of PULP.\n");
  } 

  return 0;
}

/**
 * Print information about the reserved virtual memory on the host
 */
void pulp_print_v_addr(PulpDev *pulp)
{
  // check the reservation
  printf("\nMemory map of the process:\n");
  printf("# cat /proc/getpid()/maps\n");
  
  char cmd[20];
  sprintf(cmd,"cat /proc/%i/maps",getpid());
  system(cmd);
  
  // check wether the reservation contributes to the kernels overcommit accounting -> Committed_AS
  printf("\nInformation about the system's memory:\n");
  printf("# cat /proc/meminfo\n");
  system("cat /proc/meminfo");

  return;
}

/**
 * Read 32 bits
 * @base_addr : virtual address pointer to base address 
 * @off       : offset
 * @off_type  : type of the offset, 'b' = byte offset, else word offset
 */
int pulp_read32(unsigned *base_addr, unsigned off, char off_type)
{
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

/**
 * Write 32 bits
 * @base_addr : virtual address pointer to base address 
 * @off       : offset
 * @off_type  : type of the offset, 'b' = byte offset, else word offset
 */
void pulp_write32(unsigned *base_addr, unsigned off, char off_type, unsigned value)
{
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

/**
 * Memory map the device to virtual user space using the mmap()
 * syscall
 * @pulp: pointer to the PulpDev structure
 */
int pulp_mmap(PulpDev *pulp)
{
  int offset;

  /*
   *  open the device
   */
  pulp->fd = open("/dev/PULP", O_RDWR | O_SYNC);
  if (pulp->fd < 0) {
    printf("ERROR: Opening failed \n");
    return -ENOENT;
  }

  /*
   *  do the different remappings
   */
  // PULP internals
  // Clusters
  offset = 0; // start of clusters
  pulp->clusters.size = CLUSTERS_SIZE_B;
  
  pulp->clusters.v_addr = mmap(NULL,pulp->clusters.size,
			       PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->clusters.v_addr == MAP_FAILED) {
    printf("MMAP failed for clusters.\n");
    return -EIO;
  }
  else {
     printf("Clusters mapped to virtual user space at %p.\n",pulp->clusters.v_addr);
  }

  // SOC_PERIPHERALS
  offset = CLUSTERS_SIZE_B; // start of peripherals
  pulp->soc_periph.size = SOC_PERIPHERALS_SIZE_B;
 
  pulp->soc_periph.v_addr = mmap(NULL,pulp->soc_periph.size,
				 PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->soc_periph.v_addr == MAP_FAILED) {
    printf("MMAP failed for SoC peripherals.\n");
    return -EIO;
  }
  else {
    printf("SoC peripherals mapped to virtual user space at %p.\n",pulp->soc_periph.v_addr);
  }

  // Mailbox
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B; // start of mailbox
  pulp->mailbox.size = MAILBOX_SIZE_B;
 
  pulp->mailbox.v_addr = mmap(NULL,pulp->mailbox.size,
			      PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->mailbox.v_addr == MAP_FAILED) {
    printf("MMAP failed for Mailbox.\n");
    return -EIO;
  }
  else {
    printf("Mailbox mapped to virtual user space at %p.\n",pulp->mailbox.v_addr);
  }
 
  // L2
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B; // start of L2
  pulp->l2_mem.size = L2_MEM_SIZE_B;
 
  pulp->l2_mem.v_addr = mmap(NULL,pulp->l2_mem.size,
			     PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
 
  if (pulp->l2_mem.v_addr == MAP_FAILED) {
    printf("MMAP failed for L2 memory.\n");
    return -EIO;
  }
  else {
     printf("L2 memory mapped to virtual user space at %p.\n",pulp->l2_mem.v_addr);
  }

  // Platform
  // L3
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B; // start of L3
  pulp->l3_mem.size = L3_MEM_SIZE_B;
    
  pulp->l3_mem.v_addr = mmap(NULL,pulp->l3_mem.size,
			     PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->l3_mem.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("Shared L3 memory mapped to virtual user space at %p.\n",pulp->l3_mem.v_addr);
  }
 
  // PULP external
  // GPIO
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B; // start of GPIO
  pulp->gpio.size = H_GPIO_SIZE_B;
    
  pulp->gpio.v_addr = mmap(NULL,pulp->gpio.size,
			   PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->gpio.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("GPIO memory mapped to virtual user space at %p.\n",pulp->gpio.v_addr);
  }

  // CLKING
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B; // start of Clking
  pulp->clking.size = CLKING_SIZE_B;
    
  pulp->clking.v_addr = mmap(NULL,pulp->clking.size,
			     PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->clking.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("Clock Manager memory mapped to virtual user space at %p.\n",pulp->clking.v_addr);
  }

  // STDOUT
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B; // start of Stdout
  pulp->stdout.size = STDOUT_SIZE_B;
    
  pulp->stdout.v_addr = mmap(NULL,pulp->stdout.size,
			     PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->stdout.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("Stdout memory mapped to virtual user space at %p.\n",pulp->stdout.v_addr);
  }

  // RAB config
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B; // start of RAB config
  pulp->rab_config.size = RAB_CONFIG_SIZE_B;
    
  pulp->rab_config.v_addr = mmap(NULL,pulp->rab_config.size,
				 PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->rab_config.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("RAB config memory mapped to virtual user space at %p.\n",pulp->rab_config.v_addr);
  }

  // Zynq
  // SLCR
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B; // start of SLCR
  pulp->slcr.size = SLCR_SIZE_B;
    
  pulp->slcr.v_addr = mmap(NULL,pulp->slcr.size,
			   PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->slcr.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("Zynq SLCR memory mapped to virtual user space at %p.\n",pulp->slcr.v_addr);
  }

  // MPCore
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B
    + SLCR_SIZE_B; // start of MPCore
  pulp->mpcore.size = MPCORE_SIZE_B;
    
  pulp->mpcore.v_addr = mmap(NULL,pulp->mpcore.size,
			     PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->mpcore.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return -EIO;
  }
  else {
    printf("Zynq MPCore memory mapped to virtual user space at %p.\n",pulp->mpcore.v_addr);
  }

  return 0;
}

/**
 * Undo the memory mapping of the device to virtual user space using
 * the munmap() syscall
 * @pulp: pointer to the PulpDev structure
 */
int pulp_munmap(PulpDev *pulp)
{
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

/**
 * Set the clock frequency of PULP, only do this at startup of PULP!!! 
 * @pulp:         pointer to the PulpDev structure
 * @des_freq_mhz: desired frequency in MHz
 */
int pulp_clking_set_freq(PulpDev *pulp, unsigned des_freq_mhz)
{
  int freq_mhz = des_freq_mhz - (des_freq_mhz % 5);
  if(freq_mhz <= 0)
    freq_mhz = 5;
  else if(freq_mhz >= 200)
    freq_mhz = 200;

  int divclk_divide = 1;
  int clkfbout_mult = 12;
  int clkout0_divide = 600/freq_mhz;

  // config DIVCLK_DIVIDE, CLKFBOUT_MULT, CLKFBOUT_FRAC, CLKFBOUT_PHASE
  unsigned value;
  value = 0x04000000 + 0x100*clkfbout_mult + 0x1*divclk_divide;
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_0_OFFSET_B,'b',value);
  
  // config CLKOUT0: DIVIDE, FRAC, FRAC_EN
  value = 0x00040000 + 0x1*clkout0_divide;
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_2_OFFSET_B,'b',value);

  // check status
  if ( pulp_read32(pulp->clking.v_addr,CLKING_STATUS_REG_OFFSET_B,'b') & 0x0 ) {
    printf("ERROR: Clock manager not locked, cannot reconfigure clocks.\n");
    return -EBUSY;
  }

  // start reconfiguration
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_23_OFFSET_B,'b',0x7);
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_23_OFFSET_B,'b',0x2);
 
  return freq_mhz;
}

/**
 * Initialize the memory mapped device 
 * @pulp: pointer to the PulpDev structure
 */
int pulp_init(PulpDev *pulp)
{
  // set fetch enable to 0, disable reset
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x80000000);

  // RAB setup
  // port 0: Host -> PULP
  pulp_rab_req(pulp,L2_MEM_H_BASE_ADDR,L2_MEM_SIZE_B,0x7,0,0xFF,0xFF);     // L2
  //pulp_rab_req(pulp,MAILBOX_H_BASE_ADDR,MAILBOX_SIZE_B,0x7,0,0xFF,0xFF); // Mailbox, Interface 0
  pulp_rab_req(pulp,MAILBOX_H_BASE_ADDR,MAILBOX_SIZE_B*2,0x7,0,0xFF,0xFF); // Mailbox, Interface 0 and Interface 1
  pulp_rab_req(pulp,PULP_H_BASE_ADDR,0x10000,0x7,0,0xFF,0xFF);             // TCDM
  // port 1: PULP -> Host
  pulp_rab_req(pulp,L3_MEM_BASE_ADDR,L3_MEM_SIZE_B,0x7,1,0xFF,0xFF);       // L3 memory (contiguous)
  
  // enable mailbox interrupts
  pulp_write32(pulp->mailbox.v_addr,MAILBOX_IE_OFFSET_B,'b',0x6);
 
  return 0;
}

/**
 * Request a remapping (one or more RAB slices)
 * @pulp      : pointer to the PulpDev structure
 * @addr_start: (virtual) start address
 * @size_b    : size of the remapping in bytes
 * @prot      : protection flags, one bit each for write, read, and enable
 * @port      : RAB port, 0 = Host->PULP, 1 = PULP->Host
 * @date_exp  : expiration date of the mapping
 * @date_cur  : current date, used to check for suitable slices
 */
int pulp_rab_req(PulpDev *pulp, unsigned addr_start, unsigned size_b, 
		 unsigned char prot, unsigned char port,
		 unsigned char date_exp, unsigned char date_cur)
{
  unsigned request[3];

  // setup the request
  request[0] = 0;
  RAB_SET_PROT(request[0], prot);
  RAB_SET_PORT(request[0], port);
  RAB_SET_DATE_EXP(request[0], date_exp);
  RAB_SET_DATE_CUR(request[0], date_cur);
  request[1] = addr_start;
  request[2] = size_b;
  
  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_REQ,request);
  
  return 0;
} 

/**
 * Free RAB slices
 * @pulp      : pointer to the PulpDev structure
 * @date_cur  : current date, 0 = free all slices
 */
void pulp_rab_free(PulpDev *pulp, unsigned char date_cur) {
  
  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_FREE,(unsigned)date_cur);
  
  return;
} 

/**
 * Setup a DMA transfer using the Zynq PS DMA engine
 * @pulp      : pointer to the PulpDev structure
 * @addr_l3   : virtual address in host's L3
 * @addr_pulp : physical address in PULP, so far, only L2 tested
 * @size_b    : size in bytes
 * @host_read : 0: Host -> PULP, 1: PULP -> Host (not tested)
 */
int pulp_dma_xfer(PulpDev *pulp,
		  unsigned addr_l3, unsigned addr_pulp, unsigned size_b,
		  unsigned host_read)
{ 
  unsigned request[3];
  
  // check & process arguments
  if (size_b >> 31) {
    printf("ERROR: Requested transfer size too large - cannot encode DMA transfer direction.\n ");
    return -EINVAL;
  }
  else if (host_read) {
    BF_SET(size_b,1,31,1);
  }
  
  // setup the request
  request[0] = addr_l3;
  request[1] = addr_pulp;
  request[2] = size_b;

  // make the request
  ioctl(pulp->fd,PULP_IOCTL_DMAC_XFER,request);

  return 0;
}

/**
 * Setup a DMA transfer using the Zynq PS DMA engine
 * @pulp : pointer to the PulpDev structure
 * @task : pointer to the TaskDesc structure
 */
int pulp_omp_offload_task(PulpDev *pulp, TaskDesc *task) {

  int i,n_idxs;
  unsigned * data_idxs;
 
  /*
   * RAB setup
   */
  data_idxs = (unsigned *)malloc(task->n_data*sizeof(unsigned));
  if ( data_idxs == NULL ) {
    printf("ERROR: Malloc failed for data_idxs.\n");
    return -ENOMEM;
  }
  // only remap addresses belonging to data elements larger than 32 bit
  n_idxs = pulp_offload_get_data_idxs(task, &data_idxs);
  
  // RAB setup
  pulp_offload_rab_setup(pulp, task, &data_idxs, n_idxs);
    
  // Pass data descriptor to PULP
  pulp_offload_pass_desc(pulp, task, &data_idxs);
  
  // free memory
  free(data_idxs);

  // check the virtual addresses
  if (DEBUG_LEVEL > 2) {
    for (i=0;i<task->n_data;i++) {
      printf("ptr_%d %#x\n",i,(unsigned) (task->data_desc[i].ptr));
    }
  }

  /*
   * offload
   */
  // for now: simple binary offload
  // prepare binary
  char * bin_name;
  bin_name = (char *)malloc((strlen(task->name)+4)*sizeof(char));
  if (!bin_name) {
    printf("ERROR: Malloc failed for bin_name.\n");
    return -ENOMEM;
  }
  strcpy(bin_name,task->name);
  strcat(bin_name,".bin");
  
  // read in binary
  FILE *fp;
  if((fp = fopen(bin_name, "r")) == NULL) {
    printf("BIN ERROR\n");
    return 1;
  }
  int sz, nsz;
  unsigned *bin, *bin_rv;
  fseek(fp, 0L, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  bin = (unsigned *) malloc(sz*sizeof(char));
  bin_rv = (unsigned *) malloc(sz*sizeof(char));
  if((nsz = fread(bin, sizeof(char), sz, fp)) != sz)
    printf("Read only %d bytes in binary.\n", nsz);
  fclose(fp);
  
  // reverse endianness (PULPv2 is big-endian)
  for(i=0; i<nsz/4; i++) {
    bin_rv[i] = ((bin[i] & 0x000000ff) << 24 ) |
                ((bin[i] & 0x0000ff00) <<  8 ) |
                ((bin[i] & 0x00ff0000) >>  8 ) |
                ((bin[i] & 0xff000000) >> 24 );
  } 

  // write binary to L2
  for (i=0; i<nsz/4; i++)
    pulp->l2_mem.v_addr[i] = bin_rv[i];

  // reset sync address
  pulp_write32(pulp->l2_mem.v_addr,0xFFF8,'b',0);

  // start execution
  printf("Starting program execution.\n");
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x800000ff);
  
  // poll l2_mem address for finish
  volatile int done;
  done = 0;
  while (!done) {
    done = pulp_read32(pulp->l2_mem.v_addr,0xFFF8,'b');
    usleep(1000);
    //printf("Waiting...\n");
  }
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x80000000);
  
  // reset sync address
  pulp_write32(pulp->l2_mem.v_addr,0xFFF8,'b',0);

  // -> dynamic linking here?
  
  // tell PULP where to get the binary from and to start
  // -> write address to mailbox

  // if sync, wait for PULP to finish
  // else configure the driver to listen for the interrupt and which process to wakeup

  return 0;
}

/**
 * Reset PULP
 * @pulp : pointer to the PulpDev structure
 */
void pulp_reset(PulpDev *pulp)
{
  unsigned slcr_value;

  // reset using GPIO register
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x00000000);
  usleep(100);
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x80000000);

  // FPGA reset control register
  slcr_value = pulp_read32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b');
  
  // extract the FPGA_OUT_RST bits
  slcr_value = slcr_value & 0xF;
  
  // enable reset
  pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b',
	       slcr_value | (0x1 << SLCR_FPGA_OUT_RST));
    
  // wait
  usleep(100);
    
  // disable reset
  pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', 
	       slcr_value & (0xF & (0x0 << SLCR_FPGA_OUT_RST)));
}

/**
 * Load binary to L2 and boot PULP. Not yet uses the Zynq PS DMA
 * engine.  
 * @pulp : pointer to the PulpDev structure 
 * @task : pointer to the TaskDesc structure
 */
int pulp_boot(PulpDev *pulp, TaskDesc *task)
{
  int i;

  // prepare binary reading
  char * bin_name;
  bin_name = (char *)malloc((strlen(task->name)+4)*sizeof(char));
  if (!bin_name) {
    printf("ERROR: Malloc failed for bin_name.\n");
    return -ENOMEM;
  }
  strcpy(bin_name,task->name);
  strcat(bin_name,".bin");
  
  // read in binary
  FILE *fp;
  if((fp = fopen(bin_name, "r")) == NULL) {
    printf("ERROR: Could not open PULP binary.\n");
    return -ENOENT;
  }
  int sz, nsz;
  unsigned *bin, *bin_rv;
  fseek(fp, 0L, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  bin = (unsigned *) malloc(sz*sizeof(char));
  bin_rv = (unsigned *) malloc(sz*sizeof(char));
  if((nsz = fread(bin, sizeof(char), sz, fp)) != sz)
    printf("ERROR: Red only %d bytes in binary.\n", nsz);
  fclose(fp);
  
  // reverse endianness (PULPv2 is big-endian)
  for(i=0; i<nsz/4; i++) {
    bin_rv[i] = ((bin[i] & 0x000000ff) << 24 ) |
                ((bin[i] & 0x0000ff00) <<  8 ) |
                ((bin[i] & 0x00ff0000) >>  8 ) |
                ((bin[i] & 0xff000000) >> 24 );
  } 

  // write binary to L2
  for (i=0; i<nsz/4; i++)
    pulp->l2_mem.v_addr[i] = bin_rv[i];

  // start execution
  printf("Starting program execution.\n");
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x800000ff);

  return 0;
}

/**
 * Allocate memory in contiguous L3
 * @pulp:   pointer to the PulpDev structure
 * @size_b: size in Bytes of the requested chunk
 * @p_addr: pointer to store the physical address to
 *
 * ATTENTION: This function can only allocate each address once!
 *
 */
unsigned int pulp_l3_malloc(PulpDev *pulp, size_t size_b, unsigned *p_addr)
{
  unsigned v_addr;

  // round l3_offset to next higher 64-bit word -> required for 64-bit PULP DMA
  if (pulp->l3_offset & 0x7) {
    pulp->l3_offset = (pulp->l3_offset & 0xFFFFFFF8) + 0x8;
  }

  if ( (pulp->l3_offset + size_b) >= L3_MEM_SIZE_B) {
    printf("ERROR: out of contiguous L3 memory.\n");
    *p_addr = 0;
    return 0;
  }
  
  v_addr = (unsigned int)pulp->l3_mem.v_addr + pulp->l3_offset;
  *p_addr = L3_MEM_BASE_ADDR + pulp->l3_offset;
  
  pulp->l3_offset += size_b;

  if (DEBUG_LEVEL > 2) {
    printf("Host virtual address = %#x \n",v_addr);
    printf("PMCA physical address = %#x \n",*p_addr);
  }
  
  return v_addr;
}

/**
 * Free memory previously allocated in contiguous L3
 * @pulp:   pointer to the PulpDev structure
 * @v_addr: pointer to unsigned containing the virtual address
 * @p_addr: pointer to unsigned containing the physical address
 *
 * ATTENTION: This function does not do anything!
 *
 */
void pulp_l3_free(PulpDev *pulp, unsigned v_addr, unsigned p_addr)
{
  return;
}

/**
 * Find out which shared data elements to pass by reference. 
 * @task :     pointer to the TaskDesc structure
 * @data_idxs: pointer to array marking the elements to pass by reference
 */
int pulp_offload_get_data_idxs(TaskDesc *task, unsigned **data_idxs) {
  
  int i, n_data, n_idxs, size_b;

  n_data = task->n_data;
  n_idxs = 0;
  size_b = sizeof(unsigned);


  for (i=0; i<n_data; i++) {
    if ( task->data_desc[i].size > size_b ) {
      (*data_idxs)[i] = 1;
      n_idxs++;
    }
    else
      (*data_idxs)[i] = 0;
  }

  return n_idxs;
}

/**
 * Try to reorder the shared data elements to minimize the number of
 * calls to the driver. Call the driver to set up RAB slices.
 * @task:      pointer to the TaskDesc structure
 * @data_idxs: pointer to array marking the elements to pass by reference
 */
int pulp_offload_rab_setup(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs, int n_idxs)
{
  int i, j;
  unsigned char prot, port;
  unsigned char date_cur, date_exp;

  unsigned n_data, n_data_int, gap_size, temp;
  unsigned * v_addr_int;
  unsigned * size_int;
  unsigned * order;

  // !!!!TO DO: check type and set protections!!!
  prot = 0x7; 
  port = 1;   // PULP -> Host
  
  n_data = task->n_data;
  n_data_int = 1;

  date_cur = (unsigned char)(task->task_id + 1);
  date_exp = (unsigned char)(task->task_id + 3);

  // memory allocation
  v_addr_int = (unsigned *)malloc((size_t)n_idxs*sizeof(unsigned));
  size_int = (unsigned *)malloc((size_t)n_idxs*sizeof(unsigned));
  order = (unsigned *)malloc((size_t)n_idxs*sizeof(unsigned));
  if (!v_addr_int | !size_int | !order) {
    printf("Malloc failed for RAB setup\n");
    return -ENOMEM;
  }
  j=0;
  for (i=0;i<n_data;i++) {
    if ( (*data_idxs)[i] ) {
      order[j] = i;
      j++;
    }
  }

  // order the elements - bubble sort
  for (i=n_idxs;i>1;i--) {
    for (j=0;j<i-1;j++) {
      if (task->data_desc[j].ptr > task->data_desc[j+1].ptr) {
	temp = order[j];
	order[j] = order[j+1];
	order[j+1] = temp;
      }
    }
  }
  if (DEBUG_LEVEL > 2) {
    printf("Reordered %d data elements: \n",n_idxs);
    for (i=0;i<n_idxs;i++) {
      printf("%d \t %#x \t %#x \n", order[i],
	     (unsigned)task->data_desc[order[i]].ptr,
	     (unsigned)task->data_desc[order[i]].size);
    }
  }

  // determine the number of remappings/intervals to request
  v_addr_int[0] = (unsigned)task->data_desc[order[0]].ptr; 
  size_int[0] = (unsigned)task->data_desc[order[0]].size; 
  for (i=1;i<n_idxs;i++) {
    j = order[i];
    gap_size = (unsigned)task->data_desc[j].ptr - (v_addr_int[n_data_int-1]
						      + size_int[n_data_int-1]);
    // !!!!TO DO: check protections, check dates!!!
    if ( gap_size > RAB_CONFIG_MAX_GAP_SIZE_B ) { 
      // the gap is too large, create a new mapping
      n_data_int++;
      v_addr_int[n_data_int-1] = (unsigned)task->data_desc[j].ptr;
      size_int[n_data_int-1] = (unsigned)task->data_desc[j].size;
    }
    else {
      // extend the previous mapping
      size_int[n_data_int-1] += (gap_size + task->data_desc[j].size);  
    }
  }

  // setup the RAB
  if (DEBUG_LEVEL > 2) {
    printf("Requesting %d remapping(s):\n",n_data_int);
  }
  for (i=0;i<n_data_int;i++) {
    if (DEBUG_LEVEL > 2) {
      printf("%d \t %#x \t %#x \n",i,v_addr_int[i],size_int[i]);
      sleep(1);
    }
    pulp_rab_req(pulp, v_addr_int[i], size_int[i], prot, port, date_exp, date_cur);
  }

  // free memory
  free(v_addr_int);
  free(size_int);
  free(order);
  
  return 0;
}

/**
 * Pass the descriptors of the shared data elements to PULP.
 * @pulp:      pointer to the PulpDev structure
 * @task:      pointer to the TaskDesc structure
 * @data_idxs: pointer to array marking the elements to pass by reference
 */
int pulp_offload_pass_desc(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs)
{
  int i, timeout, us_delay;
  unsigned status, n_data;

  n_data = task->n_data;

  us_delay = 100;

  if (DEBUG_LEVEL > 2) {
    printf("Mailbox status = %#x.\n",pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b'));
  }

  for (i=0;i<n_data;i++) {
    // check if mailbox is full
    if ( pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2 ) {
      timeout = 1000;
      status = 0;
      // wait for not full or timeout
      while ( status && (timeout > 0) ) {
	usleep(us_delay);
	timeout--;
	status = (pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2);
      }
      if ( status ) {
	printf("ERROR: mailbox timeout.\n");
	return i;
      } 
    }
    
    // mailbox is ready to receive
    if ( (*data_idxs)[i] ) {
      // pass data element by reference
      pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',
		   (unsigned)(task->data_desc[i].ptr));
      if (DEBUG_LEVEL > 2)
	printf("Element %d: wrote %#x to mailbox.\n",i,(unsigned) (task->data_desc[i].ptr));
    }
    else {
      // pass data element by value
      pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',
		   *(unsigned *)(task->data_desc[i].ptr));
      if (DEBUG_LEVEL > 2)
	printf("Element %d: wrote %#x to mailbox.\n",i,*(unsigned*)(task->data_desc[i].ptr));
    }    
  }
  
  if (DEBUG_LEVEL > 1) {
    printf("Passed %d of %d data elements to PULP.\n",i,n_data);
  }

  return i;
}

/**
 * Get back the shared data elements from PULP that were passed by value.
 * @pulp:      pointer to the PulpDev structure
 * @task:      pointer to the TaskDesc structure
 * @data_idxs: pointer to array marking the elements to pass by reference
 */
int pulp_offload_get_desc(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs, int n_idxs)
{
  int i,j, timeout, us_delay, n_data;
  unsigned status;

  us_delay = 10;
  j = 0;

  if (DEBUG_LEVEL > 2) {
    printf("Mailbox status = %#x.\n",pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b'));
  }

  n_data = task->n_data;

  for (i=0; i<n_data; i++) {
    // check if argument has been passed by value
    if ( (*data_idxs)[i] == 0 ) {
      // check if mailbox is empty
      if ( pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x1 ) {
	timeout = 100000;
	status = 1;
	// wait for not empty or timeout
	while ( status && (timeout > 0) ) {
	  usleep(us_delay);
	  timeout--;
	  status = (pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x1);
	}
	if ( status ) {
	  printf("ERROR: mailbox timeout.\n");
	  if (DEBUG_LEVEL > 1) {
	    printf("Got back %d of %d data elements from PULP.\n",j,n_data-n_idxs);
	  }
	  return j;
	} 
      }
     
      // mailbox contains data
      *(unsigned *)(task->data_desc[i].ptr) = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
      j++;
    }
  }
  
  if (DEBUG_LEVEL > 1) {
    printf("Got back %d of %d data elements from PULP.\n",j,n_data-n_idxs);
  }

  return j;
}

/**
 * Offload a new task to PULP, set up RAB slices and pass descriptors
 * to PULP. 
 * @pulp: pointer to the PulpDev structure 
 * @task: pointer to the TaskDesc structure
 */
int pulp_offload_out(PulpDev *pulp, TaskDesc *task)
{
  int err, n_idxs;
  unsigned *data_idxs;
  data_idxs = (unsigned *)malloc(task->n_data*sizeof(unsigned));
  if ( data_idxs == NULL ) {
    printf("ERROR: Malloc failed for data_idxs.\n");
    return -ENOMEM;
  }

  // only remap addresses belonging to data elements larger than 32 bit
  n_idxs = pulp_offload_get_data_idxs(task, &data_idxs);
  
  // RAB setup
  err = pulp_offload_rab_setup(pulp, task, &data_idxs, n_idxs);
  if (err) {
    printf("ERROR: pulp_offload_rab_setup failed.\n");
    return err;
  }

  // Pass data descriptor to PULP
  err = pulp_offload_pass_desc(pulp, task, &data_idxs);
  if ( err != task->n_data ) {
    printf("ERROR: pulp_offload_pass_desc failed.\n");
    return err;
  }

  // free memory
  free(data_idxs);

  return 0;
}

/**
 * Finish a task offload, clear RAB slices and get back descriptors
 * passed by value.
 * @pulp: pointer to the PulpDev structure 
 * @task: pointer to the TaskDesc structure
 */
int pulp_offload_in(PulpDev *pulp, TaskDesc *task)
{
  unsigned char date_cur;
  int err, n_idxs;
  unsigned *data_idxs;
  data_idxs = (unsigned *)malloc(task->n_data*sizeof(unsigned));
  if ( data_idxs == NULL ) {
    printf("ERROR: Malloc failed for data_idxs.\n");
    return -ENOMEM;
  }

  // read back data elements with sizes up to 32 bit from mailbox
  n_idxs = pulp_offload_get_data_idxs(task, &data_idxs);
  
  // free rab slices
  date_cur = (unsigned char)(task->task_id + 4);
  pulp_rab_free(pulp, date_cur);

  // fetch values of data elements passed by value
  err = pulp_offload_get_desc(pulp, task, &data_idxs, n_idxs);
  if ( err != (task->n_data - n_idxs) ) {
    printf("ERROR: pulp_offload_get_desc failed.\n");
    return err;
  }

  // free memory
  free(data_idxs);

  return 0;
}

/**
 * Start offload execution on PULP.
 * @pulp: pointer to the PulpDev structure 
 * @task: pointer to the TaskDesc structure
 */
int pulp_offload_start(PulpDev *pulp, TaskDesc *task)
{
  int timeout, us_delay;
  unsigned status;

  us_delay = 100;
  
  if (DEBUG_LEVEL > 2) {
    printf("Mailbox status = %#x.\n",pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b'));
  }
  
  // check if mailbox is empty
  if ( pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x1 ) {
    timeout = 100000;
    status = 1;
    // wait for not empty or timeout
    while ( status && (timeout > 0) ) {
      usleep(us_delay);
      timeout--;
      status = (pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x1);
    }
    if ( status ) {
      printf("ERROR: mailbox timeout.\n");
      return -ETIMEDOUT;
    } 
  }
  
  // read status
  status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_RDDATA_OFFSET_B, 'b');
  if ( status != PULP_READY ) {
    printf("ERROR: PULP status not ready. PULP status = %#x.\n",status);
    return -EBUSY;
  }

  //// check mailbox is full
  //if ( pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2 ) {
  //  printf("ERROR: PULP mailbox full.\n");
  //  return -EXFULL;
  //} 
  //
  //// start execution
  //pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', PULP_START);

  return 0;
}

/**
 * Wait for an offloaded task to finish on PULP.
 * @pulp: pointer to the PulpDev structure 
 * @task: pointer to the TaskDesc structure
 */
int pulp_offload_wait(PulpDev *pulp, TaskDesc *task)
{
  int timeout, us_delay;
  unsigned status;

  us_delay = 10;

  // check if mailbox is empty
  if ( pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x1 ) {
    timeout = 100000;
    status = 1;
    // wait for not empty or timeout
    while ( status && (timeout > 0) ) {
      usleep(us_delay);
      timeout--;
      status = (pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x1);
    }
    if ( status ) {
      printf("ERROR: mailbox timeout.\n");
      return -ETIMEDOUT;
    } 
  }

  // read status
  status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_RDDATA_OFFSET_B, 'b');
  if ( status != PULP_DONE ) {
    printf("ERROR: PULP status not done. PULP status = %#x.\n",status);
    return -EBUSY;
  }

  return 0;
}

////
//int pulp_offload_out_contiguous(PulpDev *pulp,
//				TaskDesc *desc, omp_offload_t **omp_offload, uint32_t *fomp_offload)
//// ompOffload_desc_t *desc, omp_offload_t **omp_offload, uint32_t *fomp_offload)
//{
//  int i;
//
//  omp_offload_t *new_offload = (omp_offload_t *)pulp_l3_malloc(pulp, sizeof(omp_offload_t), fomp_offload);
//  
//  /* --- Prepare the Kernel Command --- */
//  /* Resolve shared variables pointers to pass as argument */
//  new_offload->kernelInfo.n_data = desc->n_data;
//    
//  if(new_offload->kernelInfo.n_data) {
//    uint32_t fabricArgs;
//
//    //Host array for fabric args
//    new_offload->kernelInfo.host_args_ptr 
//      = (unsigned int *) pulp_l3_malloc(pulp, new_offload->kernelInfo.n_data*sizeof(unsigned int), &fabricArgs);
//    //Host array for host data pointers
//    new_offload->kernelInfo.hdata_ptrs    
//      = (unsigned int *) malloc(new_offload->kernelInfo.n_data*sizeof(unsigned int));
//    //Fabric Args
//    new_offload->kernelInfo.args = (unsigned int *) fabricArgs;
//      
//    data_desc_t *datad = (data_desc_t *) desc->data;
//    new_offload->kernelInfo.data = (data_desc_t *) desc->data;
//      
//    // memory allocation in contiguous L3 memory for IN and OUT
//    // copy to contiguous L3 memory for IN 
//    for (i = 0; i < new_offload->kernelInfo.n_data; i++) {
//      
//      int data_size, data_ptr, data_type;
//      uint32_t fabricCpy;
//            
//      data_type = datad[i].type;
//      data_size = datad[i].size;
//      data_ptr  = (uint32_t) datad[i].ptr;
//
//      switch(data_type)	{
//      case 0:
//	//INOUT
//	// vogelpi
//	//new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)p2012_socmem_alloc(data_size, &fabricCpy);
//	new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)pulp_l3_malloc(pulp, data_size, &fabricCpy);
//	new_offload->kernelInfo.host_args_ptr[i]  = fabricCpy;
//	memcpy((void *) new_offload->kernelInfo.hdata_ptrs[i], (void *) data_ptr ,data_size);
//	break;
//                    
//      case 1:
//	//IN
//	// vogelpi
//	//new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)p2012_socmem_alloc(data_size, &fabricCpy);
//	new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)pulp_l3_malloc(pulp, data_size, &fabricCpy);
//	new_offload->kernelInfo.host_args_ptr[i]  = fabricCpy;
//	memcpy((void *) new_offload->kernelInfo.hdata_ptrs[i], (void *) data_ptr ,data_size);
//	break;
//                    
//      case 2:
//	//OUT
//	// vogelpi
//	//new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)p2012_socmem_alloc(data_size, &fabricCpy);
//	new_offload->kernelInfo.hdata_ptrs[i] = (unsigned int)pulp_l3_malloc(pulp, data_size, &fabricCpy);
//	new_offload->kernelInfo.host_args_ptr[i]  = fabricCpy;
//	break;
//                    
//      default:
//	//NONE
//	new_offload->kernelInfo.host_args_ptr[i] = (unsigned int) datad[i].ptr;
//	break;
//      }
//    }
//  }
//  else {
//    new_offload->kernelInfo.host_args_ptr = 0x0;
//    new_offload->kernelInfo.args          = 0x0;
//  }
//
//  /* Attach Kernel Binary */
//  new_offload->kernelInfo.binaryHandle = (int) desc->name;
//    
//  /* Set the number of clusters requested */
//  new_offload->kernelInfo.n_clusters = desc->n_clusters;
//    
//  /* Update some more information */
//  //new_offload->hostUserPid         = (unsigned int ) new_offload | p2012_get_fabric_pid() | HOST_RT_PM_OMP_REQ_ID;
//  new_offload->host_ptr            = (uint32_t ) new_offload;    
//    
//  *omp_offload = new_offload;
//  
//  return 0;
//}
//  
//  // vogelpi -- trigger PMCA operation
//  //gomp_enqueue_offload(fomp_offload);
//  // poll status
//  while (1) {
//#ifndef PIPELINE // avoid race condition when accelerator is setting DONE and host is resetting it
//    if ( (pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B, 'b') == READY) | (pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B, 'b') == DONE) ) {
//#else    
//      if (pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B, 'b') == READY) {
//#endif
//	pulp_write32(pulp->mb_mem.v_addr, DMA_IN_ADDR_1_REG_OFFSET_B, 'b', new_offload->kernelInfo.host_args_ptr[i_start] );
//#if (DMA_OUT_SIZE_B > 0)
//	pulp_write32(pulp->mb_mem.v_addr, DMA_OUT_ADDR_1_REG_OFFSET_B ,'b', new_offload->kernelInfo.host_args_ptr[i_start+1] );
//#endif
//	pulp_write32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B ,'b', START);
//	new_offload->hostUserPid = 1; // specify which buffer to use in MB
//	break;
//      }
//#ifndef PIPELINE // avoid race condition when accelerator is setting DONE and host is resetting it
//      else if ( ( pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B, 'b') == READY) | (pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B, 'b') == DONE) ) {
//#else
//    else if ( pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B, 'b') == READY) {
//#endif     
//      pulp_write32(pulp->mb_mem.v_addr, DMA_IN_ADDR_2_REG_OFFSET_B, 'b', new_offload->kernelInfo.host_args_ptr[i_start] );
//#if (DMA_OUT_SIZE_B > 0)
//      pulp_write32(pulp->mb_mem.v_addr, DMA_OUT_ADDR_2_REG_OFFSET_B ,'b', new_offload->kernelInfo.host_args_ptr[i_start+1] );
//#endif
//      pulp_write32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B ,'b', START);
//      new_offload->hostUserPid = 2; // specify which buffer to use in MB
//      break;
//    }
//    else {
//      if (DEBUG_LEVEL > 2) {
//	printf("-- Status 1 = %u \t",pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B, 'b') );
//	printf("Status 2 = %u \n",pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B, 'b') );
//	sleep(1);
//      }
//      usleep(1000);
//    }
//      }
//      return 0;
//    }
//  }
//}

// qprintf stuff
#define ANSI_RESET   "\x1b[0m"

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

#define ANSI_BRED     "\x1b[31;1m"
#define ANSI_BGREEN   "\x1b[32;1m"
#define ANSI_BYELLOW  "\x1b[33;1m"
#define ANSI_BBLUE    "\x1b[34;1m"
#define ANSI_BMAGENTA "\x1b[35;1m"
#define ANSI_BCYAN    "\x1b[36;1m"

#define PULP_PRINTF(...) printf("[" ANSI_BRED "PULP" ANSI_RESET "] " __VA_ARGS__)
#define HOST_PRINTF(...) printf("[" ANSI_BGREEN "HOST" ANSI_RESET "] " __VA_ARGS__)

// from http://creativeandcritical.net/str-replace-c/
static char *replace_str2(const char *str, const char *old, const char *new) {
  char *ret, *r;
  const char *p, *q;
  size_t oldlen = strlen(old);
  size_t count, retlen, newlen = strlen(new);
  int samesize = (oldlen == newlen);

  if (!samesize) {
    for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
      count++;
    /* This is undefined if p - str > PTRDIFF_MAX */
    retlen = p - str + strlen(p) + count * (newlen - oldlen);
  } else
    retlen = strlen(str);

  if ((ret = malloc(retlen + 1)) == NULL)
    return NULL;

  r = ret, p = str;
  while (1) {
    /* If the old and new strings are different lengths - in other
     * words we have already iterated through with strstr above,
     * and thus we know how many times we need to call it - then we
     * can avoid the final (potentially lengthy) call to strstr,
     * which we already know is going to return NULL, by
     * decrementing and checking count.
     */
    if (!samesize && !count--)
      break;
    /* Otherwise i.e. when the old and new strings are the same
     * length, and we don't know how many times to call strstr,
     * we must check for a NULL return here (we check it in any
     * event, to avoid further conditions, and because there's
     * no harm done with the check even when the old and new
     * strings are different lengths).
     */
    if ((q = strstr(p, old)) == NULL)
      break;
    /* This is undefined if q - p > PTRDIFF_MAX */
    //ptrdiff_t l = q - p;
    unsigned l = q - p;
    memcpy(r, p, l);
    r += l;
    memcpy(r, new, newlen);
    r += newlen;
    p = q + oldlen;
  }
  strcpy(r, p);

  return ret;
}

void pulp_stdout_print(PulpDev *pulp, unsigned pe)
{
  PULP_PRINTF("PE %d\n",pe);

  char *string_ptr = (char *) (pulp->stdout.v_addr + STDOUT_PE_SIZE_B*pe);
  char *pulp_str = replace_str2((const char *) string_ptr, 
				"\n", "\n[" ANSI_BRED "PULP" ANSI_RESET "] ");
  PULP_PRINTF("%s\n", pulp_str);
  fflush(stdout);
}

void pulp_stdout_clear(PulpDev *pulp, unsigned pe)
{
  int i;
  for (i=0; i<(STDOUT_PE_SIZE_B/4); i++) {
    pulp_write32(pulp->stdout.v_addr,STDOUT_PE_SIZE_B*pe+i*4,'b',0);    
  }
  for (i=0; i<STDOUT_SIZE_B/4; i++) {
    pulp_write32(pulp->stdout.v_addr,i,'w',0);    
  }
}
