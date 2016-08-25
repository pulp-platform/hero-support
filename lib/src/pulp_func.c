#include "pulp_func.h"
#include "pulp_host.h"

//printf("%s %d\n",__FILE__,__LINE__);

/**
 * Reserve the virtual address space overlapping with the physical
 * address map of pulp using the mmap() syscall with MAP_FIXED and
 * MAP_ANONYMOUS
 *
 * @pulp: pointer to the PulpDev structure
 */
int pulp_reserve_v_addr(PulpDev *pulp)
{
  pulp->pulp_res_v_addr.size = PULP_SIZE_B;
  pulp->pulp_res_v_addr.v_addr = mmap((int *)PULP_BASE_REMOTE_ADDR,pulp->pulp_res_v_addr.size,
                                      PROT_NONE,MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,-1,0);
  if (pulp->pulp_res_v_addr.v_addr == MAP_FAILED) {
    printf("MMAP failed to reserve virtual addresses overlapping with physical address map of PULP.\n");
    return EIO;
  }
  else {
    printf("Reserved virtual addresses starting at %p and overlapping with physical address map of PULP. \n",
           pulp->pulp_res_v_addr.v_addr);
  }

  pulp->l3_mem_res_v_addr.size = L3_MEM_SIZE_B;
  pulp->l3_mem_res_v_addr.v_addr = mmap((int *)L3_MEM_BASE_ADDR,pulp->l3_mem_res_v_addr.size,
                                        PROT_NONE,MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,-1,0);
  if (pulp->l3_mem_res_v_addr.v_addr == MAP_FAILED) {
    printf("MMAP failed to reserve virtual addresses overlapping with physically contiguous L3 memory.\n");
    return EIO;
  }
  else {
    printf("Reserved virtual addresses starting at %p and overlapping with physically contiguous L3 memory.\n",
           pulp->l3_mem_res_v_addr.v_addr);
  }

  return 0;
}

/**
 * Free the virtual address space overlapping with the physical
 * address map of pulp using the munmap() syscall
 *
 * @pulp: pointer to the PulpDev structure
 */
int pulp_free_v_addr(PulpDev *pulp)
{  
  int status;

  printf("Freeing reserved virtual addresses overlapping with physical address map of PULP.\n");
  status = munmap(pulp->pulp_res_v_addr.v_addr,pulp->pulp_res_v_addr.size);
  if (status) {
    printf("MUNMAP failed to free reserved virtual addresses overlapping with physical address map of PULP.\n");
  }

#if PLATFORM != JUNO
  printf("Freeing reserved virtual addresses overlapping with with physically contiguous L3 memory.\n");
  status = munmap(pulp->l3_mem_res_v_addr.v_addr,pulp->l3_mem_res_v_addr.size);
  if (status) {
    printf("MUNMAP failed to free reserved virtual addresses overlapping with physically contiguous L3 memory.\n");
  }
#endif

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
 *
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
 *
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
 *
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
  pulp->mbox.size = MBOX_SIZE_B;
 
  pulp->mbox.v_addr = mmap(NULL,pulp->mbox.size,
                              PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->mbox.v_addr == MAP_FAILED) {
    printf("MMAP failed for Mailbox.\n");
    return -EIO;
  }
  else {
    printf("Mailbox mapped to virtual user space at %p.\n",pulp->mbox.v_addr);
  }
 
  // L2
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B; // start of L2
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
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B; // start of L3
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
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B
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
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B
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

  // RAB config
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B; // start of RAB config
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

#if PLATFORM != JUNO // ZYNQ
  // SLCR
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + RAB_CONFIG_SIZE_B; // start of SLCR
  pulp->slcr.size = SLCR_SIZE_B;
    
  pulp->slcr.v_addr = mmap(NULL,pulp->slcr.size,
                           PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->slcr.v_addr == MAP_FAILED) {
    printf("MMAP failed for Zynq SLCR.\n");
    return -EIO;
  }
  else {
    printf("Zynq SLCR memory mapped to virtual user space at %p.\n",pulp->slcr.v_addr);
  }

  // MPCore
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MBOX_SIZE_B + L2_MEM_SIZE_B
    + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B; // start of MPCore
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
#endif

  return 0;
}

/**
 * Undo the memory mapping of the device to virtual user space using
 * the munmap() syscall
 *
 * @pulp: pointer to the PulpDev structure
 */
int pulp_munmap(PulpDev *pulp)
{
  unsigned status;

  // undo the memory mappings
  printf("Undo the memory mappings.\n");
#if PLATFORM != JUNO
  status = munmap(pulp->mpcore.v_addr,pulp->mpcore.size);
  if (status) {
    printf("MUNMAP failed for MPCore.\n");
  }
  status = munmap(pulp->slcr.v_addr,pulp->slcr.size);
  if (status) {
    printf("MUNMAP failed for SLCR.\n");
  }
#endif
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
 *
 * clk_in   = CLKING_INPUT_FREQ_MHZ (100 or 50 MHz) multiplied to 1000 MHz
 * clk_out1 = CLKOUT0: divide 1000 MHz by CLKING_CONFIG_REG_2 -> ClkSoc_C
 * clk_out2 = CLKOUT1: divide 1000 MHz by CLKING_CONFIG_REG_5 -> ClkCluster_C
 *
 * @pulp:         pointer to the PulpDev structure
 * @des_freq_mhz: desired frequency in MHz
 */
int pulp_clking_set_freq(PulpDev *pulp, unsigned des_freq_mhz)
{
  unsigned status;
  int timeout;
  int freq_mhz = des_freq_mhz - (des_freq_mhz % 5);
  if(freq_mhz <= 0)
    freq_mhz = 5;
  else if(freq_mhz >= 200)
    freq_mhz = 200;

  // Bring the input clock to 1000 MHz
#if CLKING_INPUT_FREQ_MHZ == 50
  int divclk_divide = 1;
  int clkfbout_mult = 20;
#elif CLKING_INPUT_FREQ_MHZ == 100
  int divclk_divide = 1;
  int clkfbout_mult = 10;
#else
  #error CLKING_INPUT_FREQ_MHZ value is not supported
#endif

  // default output clock = 50 MHz
  int clkout0_divide = 1000/freq_mhz;
  int clkout0_divide_frac = ((1000 % freq_mhz) << 10)/freq_mhz;

  // config DIVCLK_DIVIDE, CLKFBOUT_MULT, CLKFBOUT_FRAC, CLKFBOUT_PHASE
  unsigned value;
  value = 0x04000000 + 0x100*clkfbout_mult + 0x1*divclk_divide;
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_0_OFFSET_B,'b',value);
  if (DEBUG_LEVEL > 3)
    printf("CLKING_CONFIG_REG_0: %#x\n",value);

  // config CLKOUT0/1: DIVIDE, FRAC, FRAC_EN
  value = 0x00040000 + 0x100*clkout0_divide_frac + 0x1*clkout0_divide;
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_2_OFFSET_B,'b',value);
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_5_OFFSET_B,'b',value);
  if (DEBUG_LEVEL > 3)
    printf("CLKING_CONFIG_REG_2/5: %#x\n",value);

  // check status
  if ( !(pulp_read32(pulp->clking.v_addr,CLKING_STATUS_REG_OFFSET_B,'b') & 0x1) ) {
    timeout = 10;
    status = 1;
    while ( status && (timeout > 0) ) {
      usleep(10000);
      timeout--;
      status = !(pulp_read32(pulp->clking.v_addr,CLKING_STATUS_REG_OFFSET_B,'b') & 0x1);
    }
    if ( status ) {
      printf("ERROR: Clock manager not locked, cannot start reconfiguration.\n");
      return -EBUSY;
    } 
  }

  // start reconfiguration
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_23_OFFSET_B,'b',0x7);
  usleep(1000);
  pulp_write32(pulp->clking.v_addr,CLKING_CONFIG_REG_23_OFFSET_B,'b',0x2);

  // check status
  if ( !(pulp_read32(pulp->clking.v_addr,CLKING_STATUS_REG_OFFSET_B,'b') & 0x1) ) {
    timeout = 10;
    status = 1;
    while ( status && (timeout > 0) ) {
      usleep(10000);
      timeout--;
      status = !(pulp_read32(pulp->clking.v_addr,CLKING_STATUS_REG_OFFSET_B,'b') & 0x1);
    }
    if ( status ) {
      printf("ERROR: Clock manager not locked, clock reconfiguration failed.\n");
      return -EBUSY;
    } 
  }

#if PLATFORM != JUNO
  // reconfigure PULP -> host UART
  int baudrate = 115200;
  char cmd[40];
  char baudrate_s[10];
  float ratio,baudrate_f;

  // compute custom baudrate
  ratio = (float)freq_mhz/(float)PULP_DEFAULT_FREQ_MHZ;
  baudrate_f = (float)baudrate * ratio;
  baudrate = (int)baudrate_f;
  sprintf(baudrate_s, "%i", baudrate);

  printf("Please configure /dev/ttyPS1 for %i baud.\n",baudrate);

  // prepare command
  //strcpy(cmd,"stty -F /dev/ttyPS1 ");  // only supports standard baudrates
  strcpy(cmd,"/media/nfs/apps/uart "); // supports non-standard baudrates 
  strcat(cmd,baudrate_s);
  strcat(cmd," 0");

  //printf("%s\n",cmd);

  // set the baudrate -- can cause crashes
  //system(cmd);
#endif

  return freq_mhz;
}

/**
 * Measure the clock frequency of PULP. Can only be executed with the
 * RAB configured to allow accessing the cluster peripherals. To
 * validate the measurement, the ZYNQ_PMM needs to be loaded for
 * access to the ARM clock counter.
 *
 * @pulp:         pointer to the PulpDev structure
 */
int pulp_clking_measure_freq(PulpDev *pulp)
{
  unsigned seconds = 1;
  unsigned limit = 0;

#if PLATFORM != JUNO
   limit = (unsigned)((float)(ARM_CLK_FREQ_MHZ*100000*1.61)*seconds);
#else
   limit = (unsigned)((float)(ARM_CLK_FREQ_MHZ*100000*1.61)*seconds);
#endif

  unsigned pulp_clk_counter, arm_clk_counter;
  unsigned zynq_pmm;

  volatile unsigned k;
  int mes_freq_mhz;
  
  if( access("/dev/ZYNQ_PMM", F_OK ) != -1 )
    zynq_pmm = 1;
  else
    zynq_pmm = 0;

  // start clock counters
  if (zynq_pmm) {
    // enable clock counter divider (by 64), reset & enable clock counter, PMCR register 
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
  }

  pulp_write32(pulp->clusters.v_addr,TIMER_STOP_OFFSET_B,'b',0x1);
  pulp_write32(pulp->clusters.v_addr,TIMER_RESET_OFFSET_B,'b',0x1);
  pulp_write32(pulp->clusters.v_addr,TIMER_START_OFFSET_B,'b',0x1);

  // wait but don't sleep
  k = 0;
  while (k<limit) {
    k++;
    k++;
    k++;
    k++;
    k++;
    k++;
    k++;
    k++;
    k++;
    k++;
  }

  // stop and read clock counters
  if (zynq_pmm) {
    // Read the counter value, PMCCNTR register
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(arm_clk_counter) : );
  }
  pulp_clk_counter = pulp_read32(pulp->clusters.v_addr,TIMER_GET_TIME_LO_OFFSET_B,'b');
 
  if (zynq_pmm) {
    mes_freq_mhz = (int)((float)pulp_clk_counter/((float)(arm_clk_counter*ARM_PMU_CLK_DIV)/ARM_CLK_FREQ_MHZ));
  }
  else {
    mes_freq_mhz = (int)((float)pulp_clk_counter/seconds/1000000);
  }

  return mes_freq_mhz;
}

/**
 * Initialize the memory mapped device 
 *
 * @pulp: pointer to the PulpDev structure
 */
int pulp_init(PulpDev *pulp)
{
  // set fetch enable to 0, set global clk enable, disable reset
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0xC0000000);

  // RAB setup
  // port 0: Host -> PULP
  pulp_rab_req(pulp,L2_MEM_H_BASE_ADDR,L2_MEM_SIZE_B,0x7,0,RAB_MAX_DATE,RAB_MAX_DATE, 0, 1);     // L2
  //pulp_rab_req(pulp,MBOX_H_BASE_ADDR,MBOX_SIZE_B,0x7,0,RAB_MAX_DATE,RAB_MAX_DATE, 0, 1); // Mailbox, Interface 0
  pulp_rab_req(pulp,MBOX_H_BASE_ADDR,MBOX_SIZE_B*2,0x7,0,RAB_MAX_DATE,RAB_MAX_DATE, 0, 1); // Mailbox, Interface 0 and Interface 1
  pulp_rab_req(pulp,PULP_H_BASE_ADDR,CLUSTERS_SIZE_B,0x7,0,RAB_MAX_DATE,RAB_MAX_DATE, 0, 1);     // TCDM + Cluster Peripherals
  // port 1: PULP -> Host
  pulp_rab_req(pulp,L3_MEM_BASE_ADDR,L3_MEM_SIZE_B,0x7,1,RAB_MAX_DATE,RAB_MAX_DATE, 0, 1);       // contiguous L3 memory

  // enable mbox interrupts
  pulp_write32(pulp->mbox.v_addr,MBOX_IE_OFFSET_B,'b',0x6);
 
  // reset the l3_offset pointer
  pulp->l3_offset = 0;

  return 0;
}

/**
 * Read n_words words from mbox, can block if the mbox does not
 * contain enough data.
 *
 * @pulp      : pointer to the PulpDev structure
 * @buffer    : pointer to read buffer
 * @n_words   : number of words to read
 */
int pulp_mbox_read(PulpDev *pulp, unsigned *buffer, unsigned n_words)
{
  int n_char, n_char_left, ret;
  ret = 1;
  n_char = 0;
  n_char_left = n_words*sizeof(buffer[0]);

  // read n_words words or until error
  while (n_char_left) {
    ret = read(pulp->fd, (char *)&buffer[n_char],n_char_left*sizeof(char));
    if (ret < 0) {
      printf("ERROR: Could not read mbox.\n");
      return ret;
    }
    n_char += ret;
    n_char_left -= ret;
  }

  return 0;
}

/**
 * Write one word to mbox.
 *
 * @pulp      : pointer to the PulpDev structure
 * @word      : word to write
 */
int pulp_mbox_write(PulpDev *pulp, unsigned word)
{
  unsigned timeout, status;
  unsigned us_delay = 100;

  // check if mbox is full
  if ( pulp_read32(pulp->mbox.v_addr, MBOX_STATUS_OFFSET_B, 'b') & 0x2 ) {
    timeout = 1000;
    status = 1;
    // wait for not full or timeout
    while ( status && (timeout > 0) ) {
      usleep(us_delay);
      timeout--;
      status = (pulp_read32(pulp->mbox.v_addr, MBOX_STATUS_OFFSET_B, 'b') & 0x2);
    }
    if ( status ) {
      printf("ERROR: mbox timeout.\n");
      return -ETIME;
    }
  } 
 
  // mbox is ready to receive
  pulp_write32(pulp->mbox.v_addr,MBOX_WRDATA_OFFSET_B,'b', word);
  if (DEBUG_LEVEL > 2)
    printf("Wrote %#x to mbox.\n",word);

  return 0;  
}

/**
 * Clear interrupt status flag in mbox. The next write of PULP will
 * again be handled by the PULP driver.
 *
 * @pulp: pointer to the PulpDev structure
 */
void pulp_mbox_clear_is(PulpDev *pulp)
{
  pulp_write32(pulp->mbox.v_addr,MBOX_IS_OFFSET_B,'b',0x7);
}

/**
 * Request a remapping (one or more RAB slices)
 *
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
                 unsigned char date_exp, unsigned char date_cur,
                 unsigned char use_acp, unsigned char rab_lvl)
{
  int status;
  unsigned request[3];

  // setup the request
  request[0] = 0;
  RAB_SET_PROT(request[0], prot);
  RAB_SET_ACP(request[0], use_acp);
  RAB_SET_PORT(request[0], port);
  RAB_SET_LVL(request[0], rab_lvl);
  RAB_SET_DATE_EXP(request[0], date_exp);
  RAB_SET_DATE_CUR(request[0], date_cur);
  request[1] = addr_start;
  request[2] = size_b;

  // make the request
  status = ioctl(pulp->fd,PULP_IOCTL_RAB_REQ,request);
  if (status) {
    printf("status = %d, errno = %d\n",status,errno);
  }

  return 0;
}

/**
 * Free RAB slices
 *
 * @pulp      : pointer to the PulpDev structure
 * @date_cur  : current date, 0 = free all slices
 */
void pulp_rab_free(PulpDev *pulp, unsigned char date_cur) {

  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_FREE,(unsigned)date_cur);
}

/**
 * Request striped remappings
 *
 * @pulp:       pointer to the PulpDev structure
 * @task:       pointer to the TaskDesc structure
 * @data_idxs:  pointer to array marking the elements to pass by reference
 * @n_elements: number of striped data elements
 */
int pulp_rab_req_striped(PulpDev *pulp, TaskDesc *task,
                         unsigned **data_idxs, int n_elements)
{
  int i,j,k;

  RabStripeReqUser stripe_request;

  stripe_request.id         = 0;
  stripe_request.n_elements = (short)n_elements;

  RabStripeElemUser * elements = (RabStripeElemUser *)
    malloc((size_t)(stripe_request.n_elements*sizeof(RabStripeElemUser)));
  if ( elements == NULL ) {
    printf("ERROR: Malloc failed for RabStripeElemUser structs.\n");
    return -ENOMEM;
  }

  stripe_request.rab_stripe_elem_user_addr = (unsigned)&elements[0];

  unsigned * addr_start;
  unsigned * addr_end;

  unsigned flags, prot;
  unsigned size_b;

  // extracted from accelerator code
  // for ROD
  unsigned R, TILE_HEIGHT;
  unsigned w, h, n_bands, band_height, odd;
  unsigned tx_band_size_in = 0, tx_band_size_in_first = 0, tx_band_size_in_last = 0;
  unsigned tx_band_size_out = 0, tx_band_size_out_last = 0;
  unsigned overlap = 0;

  // for CT
  unsigned width, height, bHeight, nbBands;
  unsigned band_size_1ch = 0, band_size_3ch = 0;

  unsigned tx_band_start = 0;
  size_b = 0;

  if ( !strcmp(task->name, "profile_rab_striping") ) { // valid for PROFILE_RAB_STR only

    for (i=0; i<stripe_request.n_elements; i++) {
      elements[i].max_stripe_size_b = MAX_STRIPE_SIZE_B;
    
      elements[i].n_stripes = (task->data_desc[i].size)/elements[i].max_stripe_size_b;
      if ( (elements[i].n_stripes * elements[i].max_stripe_size_b) < task->data_desc[i].size)
        elements[i].n_stripes++;
    }
  }
  else if ( !strcmp(task->name, "rod") ) { // valid for ROD only

    // max sizes hardcoded
    elements[0].max_stripe_size_b = 0x1100;
    elements[1].max_stripe_size_b = 0x1100;
    elements[2].max_stripe_size_b = 0xe00;  
  
    R = 3;
    TILE_HEIGHT = 10;
    w = *(unsigned *)(task->data_desc[3].ptr);
    h = *(unsigned *)(task->data_desc[4].ptr);

    n_bands = h / TILE_HEIGHT;
    band_height = TILE_HEIGHT;
    odd = h - (n_bands * band_height);
  
    tx_band_size_in       = sizeof(unsigned char)*((band_height + (R << 1)) * w);
    tx_band_size_in_first = sizeof(unsigned char)*((band_height + R) * w);
    tx_band_size_in_last  = sizeof(unsigned char)*((band_height + odd + R ) * w);
    tx_band_size_out      = sizeof(unsigned char)*( band_height * w);
    tx_band_size_out_last = sizeof(unsigned char)*((band_height + odd )* w);
  
    overlap = sizeof(unsigned char)*(R * w);
    elements[0].n_stripes = n_bands;
    elements[1].n_stripes = n_bands;
    elements[2].n_stripes = n_bands;
  }
  else if ( !strcmp(task->name, "ct") ) { // valid for CT only

    // max sizes hardcoded
    elements[0].max_stripe_size_b = 0x3000;

    width   = *(unsigned *)(task->data_desc[1].ptr);
    height  = *(unsigned *)(task->data_desc[2].ptr);
    bHeight = *(unsigned *)(task->data_desc[3].ptr);

    nbBands = (height / bHeight);
    //band_size_1ch = (width*bHeight);
    band_size_3ch = (width*bHeight*3);  
    //printf("buffer size = %#x \n",(band_size_3ch*2+band_size_1ch)*sizeof(unsigned char));

    elements[0].n_stripes = nbBands;
  }
  else if ( !strcmp(task->name, "jpeg") ) { // valid for JPEG only

    // max sizes hardcoded
    elements[0].max_stripe_size_b = 0x1000;
    elements[0].n_stripes         = 18;
  }
  else {

    elements[0].max_stripe_size_b = 0;
    elements[0].n_stripes         = 1;
    printf("ERROR: Unknown task name %s\n",task->name);

  }
  
  i = -1;
  for (k=0; k<task->n_data; k++) {

    // check if element is passed by value or if the this element is not striped over
    if ( (*data_idxs)[k] < 2 )
      continue;
    i++;
    
    if (DEBUG_LEVEL > 2) {
      printf("size_b[%d] = %#x\n",i,task->data_desc[k].size);
      printf("elements[%d].max_stripe_size_b = %#x\n",i,elements[i].max_stripe_size_b);
    }

    flags = 0;
    if      (task->data_desc[k].type == 2) // in = read
      prot = 0x2 | 0x1;
    else if (task->data_desc[k].type == 3) // out = write
      prot = 0x4 | 0x1;
    else // 4 inout = read & write
      prot = 0x4 | 0x2 | 0x1;
    RAB_SET_PROT(flags, prot);
    RAB_SET_ACP(flags, task->data_desc[k].use_acp);
     
    elements[i].id    = (unsigned char)i;               // not used right now
    elements[i].type  = (unsigned char)(*data_idxs)[k]; // != data_desc[k].type
    elements[i].flags = (unsigned char)flags;

    addr_start = (unsigned *)malloc((size_t)elements[i].n_stripes*sizeof(unsigned));
    addr_end   = (unsigned *)malloc((size_t)elements[i].n_stripes*sizeof(unsigned));
    if ( (addr_start == NULL) || (addr_end == NULL) ) {
      printf("ERROR: Malloc failed for addr_start/addr_end.\n");
      return -ENOMEM;
    }

    elements[i].stripe_addr_start = (unsigned)&addr_start[0];
    elements[i].stripe_addr_end   = (unsigned)&addr_end[0];

    // fill in stripe data
    for (j=0; j<elements[i].n_stripes; j++) {

      if ( !strcmp(task->name, "profile_rab_striping") ) {
    
        size_b = elements[i].max_stripe_size_b;
        tx_band_start = j*size_b;

      }
      else if ( !strcmp(task->name, "rod") ) {
  
        if ( (*data_idxs)[k] == 2 ) { // input elements
          if (j == 0) {
            tx_band_start = 0;
            size_b = tx_band_size_in_first;
          }
          else {
            tx_band_start = tx_band_size_in_first + tx_band_size_in * (j-1) - (overlap * (2 + (j-1)*2));
            if (j == elements[i].n_stripes )      
              size_b = tx_band_size_in_last;
            else
              size_b = tx_band_size_in;
          }
        }
        else {// 3, output elements
          tx_band_start = tx_band_size_out * j;
          if (j == elements[i].n_stripes ) 
            size_b = tx_band_size_out_last;
          else
            size_b = tx_band_size_out;
        }

      }
      else if ( !strcmp(task->name, "ct") ) {
    
        tx_band_start = j*band_size_3ch;
        size_b = band_size_3ch;

      }
      else if ( !strcmp(task->name, "jpeg") ) {
    
        tx_band_start = j*elements[i].max_stripe_size_b;
        size_b = elements[i].max_stripe_size_b;

      }
      else 
        printf("ERROR: Unknown task name %s\n",task->name);
     
      // write the address arrays
      addr_start[j] = (unsigned)(task->data_desc[k].ptr) + tx_band_start;
      addr_end[j]   = addr_start[j] + size_b;
    }

    if (DEBUG_LEVEL > 2) {
      printf("Shared Element %d: \n",k);
      printf("stripe_addr_start @ %#x\n",elements[i].stripe_addr_start);
      printf("stripe_addr_end   @ %#x\n",elements[i].stripe_addr_end);
      for (j=0; j<elements[i].n_stripes; j++) {
        if (j>2 && j<(elements[i].n_stripes-3))
          continue;
        printf("%d\t",j);
        printf("%#x - ", ((unsigned *)(elements[i].stripe_addr_start))[j]);
        printf("%#x\n",  ((unsigned *)(elements[i].stripe_addr_end))[j]);
        printf("\n");
      }
    }
  }

  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_REQ_STRIPED,(unsigned *)&stripe_request);

  // free memory
  for (i=0; i<stripe_request.n_elements; i++) {
    free((unsigned *)elements[i].stripe_addr_start);
    free((unsigned *)elements[i].stripe_addr_end);
  }
  free(elements);

  return 0;
} 

/**
 * Free striped remappings
 *
 * @pulp      : pointer to the PulpDev structure
 */
void pulp_rab_free_striped(PulpDev *pulp)
{
  
  unsigned offload_id = 0;

  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_FREE_STRIPED,offload_id);
} 

void pulp_rab_mh_enable(PulpDev *pulp, unsigned char use_acp, unsigned char rab_mh_lvl)
{
  unsigned rab_mh_cfg[2];
  rab_mh_cfg[0] = (unsigned)use_acp;
  rab_mh_cfg[1] = (unsigned)rab_mh_lvl;
  ioctl(pulp->fd,PULP_IOCTL_RAB_MH_ENA,rab_mh_cfg);
}

void pulp_rab_mh_disable(PulpDev *pulp)
{
  ioctl(pulp->fd,PULP_IOCTL_RAB_MH_DIS);
}

/**
 * Setup a DMA transfer using the Zynq PS DMA engine
 *
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
 * Offload an OpenMP task to PULP and setup the RAB
 *
 * Currently only used by profile_rab_striping, may be removed soon.
 *
 * @pulp : pointer to the PulpDev structure
 * @task : pointer to the TaskDesc structure
 */
int pulp_omp_offload_task(PulpDev *pulp, TaskDesc *task) {

  int i,n_idxs,err;
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
  err = pulp_load_bin(pulp, task->name);
  if (err) {
    printf("ERROR: Load of PULP binary failed.\n");
    return err;
  }
  pulp_exe_start(pulp);

  return 0;
}

/**
 * Reset PULP
 * @pulp : pointer to the PulpDev structure
 * @full : type of reset: 0 for PULP reset, 1 for entire FPGA
 */
void pulp_reset(PulpDev *pulp, unsigned full)
{

printf("%s %d\n",__FILE__,__LINE__);

#if PLATFORM != JUNO
  unsigned slcr_value;

  // FPGA reset control register
  slcr_value = pulp_read32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b');
 
  // extract the FPGA_OUT_RST bits
  slcr_value = slcr_value & 0xF;

  if (full) {
    // enable reset
    pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', 0xF);

    printf("%s %d\n",__FILE__,__LINE__);

    // wait
    usleep(100000);

    // disable reset
    pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', slcr_value);

    printf("%s %d\n",__FILE__,__LINE__);
  }
  else {
    // enable reset
    pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b',
                 slcr_value | (0x1 << SLCR_FPGA_OUT_RST));
    
    // wait
    usleep(100000);
    
    // disable reset
    pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', 
                 slcr_value );
#endif

    // reset using GPIO register
    pulp_write32(pulp->gpio.v_addr,0x8,'b',0x00000000);
    usleep(100000);
    pulp_write32(pulp->gpio.v_addr,0x8,'b',0xC0000000);
    usleep(100000);

    printf("%s %d\n",__FILE__,__LINE__);

#if PLATFORM != JUNO
  }
  // temporary fix: global clk enable
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0xC0000000);
#endif

  printf("%s %d\n",__FILE__,__LINE__);
}

/**
 * Boot PULP.  
 *
 * @pulp : pointer to the PulpDev structure 
 * @task : pointer to the TaskDesc structure
 */
int pulp_boot(PulpDev *pulp, TaskDesc *task)
{
  int err;

  // load the binary
  err = pulp_load_bin(pulp, task->name);
  if (err) {
    printf("ERROR: Load of PULP binary failed.\n");
    return err;
  }
  
  // start execution
  pulp_exe_start(pulp);

  return 0;
}

/**
 * Load binary to PULP. Not yet uses the Zynq PS DMA engine.
 *
 * @pulp : pointer to the PulpDev structure 
 * @name : pointer to the string containing the name of the 
 *         application to load
 */
int pulp_load_bin(PulpDev *pulp, char *name)
{
  int i;
  char * bin_name;
  int append = 0;  

  // prepare binary name
  if ( strlen(name) < 5)
    append = 1;
  else {
    char * last_dot;
    last_dot = strrchr(name, '.');
    if ( (NULL == last_dot) || (strncmp(last_dot,".bin",4)) )
      append = 1;
  }
  
  if (append) {
    bin_name = (char *)malloc((strlen(name)+4+1)*sizeof(char));
    if (!bin_name) {
      printf("ERROR: Malloc failed for bin_name.\n");
      return -ENOMEM;
    }
    strcpy(bin_name,name);
    strcat(bin_name,".bin");
  }
  else
    bin_name = name;

  printf("Loading binary file: %s\n",bin_name);

  // read in binary
  FILE *fp;
  if((fp = fopen(bin_name, "r")) == NULL) {
    printf("ERROR: Could not open PULP binary.\n");
    return -ENOENT;
  }
  int sz, nsz;
  unsigned *bin;
  fseek(fp, 0L, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  bin = (unsigned *) malloc(sz*sizeof(char));
  if((nsz = fread(bin, sizeof(char), sz, fp)) != sz)
    printf("ERROR: Red only %d bytes in binary.\n", nsz);
  fclose(fp);
  
  // write binary to L2
  for (i=0; i<nsz/4; i++)
    pulp->l2_mem.v_addr[i] = bin[i];

  // cleanup
  if (append)
    free(bin_name);

  return 0;
}

/**
 * Starts programm execution on PULP.
 *
 * @pulp : pointer to the PulpDev structure 
 */
void pulp_exe_start(PulpDev *pulp)
{

  unsigned int value = 0xC0000000;
  value |= pulp->cluster_sel;

  printf("Starting program execution.\n");
  pulp_write32(pulp->gpio.v_addr,0x8,'b',value);
}

/**
 * Stops programm execution on PULP.
 *
 * @pulp : pointer to the PulpDev structure 
 */
void pulp_exe_stop(PulpDev *pulp)
{
  printf("Stopping program execution.\n");
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0xC0000000);
}

/**
 * Polls the GPIO register for the end of computation signal for at
 * most timeous_s seconds.
 *
 * @pulp      : pointer to the PulpDev structure
 * @timeout_s : maximum number of seconds to wait for end of 
 *              computation
 */
int pulp_exe_wait(PulpDev *pulp, int timeout_s)
{
  unsigned status, gpio_eoc;
  float interval_us = 100000;
  float timeout = (float)timeout_s*1000000/interval_us;

  gpio_eoc = BF_GET(pulp_read32(pulp->gpio.v_addr,0,'b'), GPIO_EOC_0, GPIO_EOC_N-GPIO_EOC_0+1);
  status   = (gpio_eoc == pulp->cluster_sel) ? 0 : 1;

  while ( status && (timeout > 0) ) {
    usleep(interval_us);
    timeout--;
    gpio_eoc = BF_GET(pulp_read32(pulp->gpio.v_addr,0,'b'), GPIO_EOC_0, GPIO_EOC_N-GPIO_EOC_0+1);
    status   = (gpio_eoc == pulp->cluster_sel) ? 0 : 1;
  }
  if ( status ) {
    printf("ERROR: PULP execution timeout.\n");
    return -ETIME;
  }
   
  return 0;
}

/**
 * Allocate memory in contiguous L3
 *
 * @pulp:   pointer to the PulpDev structure
 * @size_b: size in Bytes of the requested chunk
 * @p_addr: pointer to store the physical address to
 *
 * ATTENTION: This function can only allocate each address once!
 *
 */
unsigned int pulp_l3_malloc(PulpDev *pulp, size_t size_b, unsigned *p_addr)
{
  unsigned int v_addr;

  // round l3_offset to next higher 64-bit word -> required for PULP DMA
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
 *
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
 *
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
 *
 * @task:      pointer to the TaskDesc structure
 * @data_idxs: pointer to array marking the elements to pass by reference
 * @n_idxs:    number of shared data elements passed by reference
 */
int pulp_offload_rab_setup(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs, int n_idxs)
{
  int i, j;
  unsigned char prot, port;
  unsigned char date_cur, date_exp;

  unsigned n_data, n_data_int, gap_size, temp;
  unsigned * v_addr_int;
  unsigned * size_int;
  unsigned char * use_acp_int;
  unsigned char * rab_lvl_int;
  unsigned * order;

  n_data_int = 1;

  // Mark striped data elements
  if ( !strcmp(task->name, "profile_rab_striping") ) { // valid for PROFILE_RAB_STR only
        
    n_data_int = 0;
    for (i=0; i<task->n_data; i++) {
      (*data_idxs)[i] = 2; // striped in
    }
    n_idxs -= task->n_data;

  }
  else if ( !strcmp(task->name, "rod") ) { // valid for ROD only

    (*data_idxs)[0] = 2; // striped in
    (*data_idxs)[1] = 2; // striped in 
    (*data_idxs)[2] = 3; // striped out (shifted)
    n_idxs -= 3;

  }
  else if ( !strcmp(task->name, "ct") ) { // valid for CT only
    
    (*data_idxs)[0] = 2; // striped in
    n_idxs -= 1;

  }
  else if ( !strcmp(task->name, "jpeg") ) { // valid for JPEG only
    
    (*data_idxs)[3] = 4; // striped inout
    n_idxs -= 1;

  }
  else {
    if ( strcmp(task->name, "face_detect") )
      printf("ERROR: Unknown task name %s\n",task->name);
  }

  // !!!!TO DO: check type and set protections!!!
  prot = 0x7; 
  port = 1;   // PULP -> Host
  
  n_data = task->n_data;

  date_cur = (unsigned char)(task->task_id + 1);
  date_exp = (unsigned char)(task->task_id + 3);

  // memory allocation
  v_addr_int = (unsigned *)malloc((size_t)n_idxs*sizeof(unsigned));
  size_int = (unsigned *)malloc((size_t)n_idxs*sizeof(unsigned));
  use_acp_int = (unsigned char *)malloc((size_t)n_idxs*sizeof(unsigned char));
  rab_lvl_int = (unsigned char *)malloc((size_t)n_idxs*sizeof(unsigned char));
  order = (unsigned *)malloc((size_t)n_idxs*sizeof(unsigned));
  if (!v_addr_int | !size_int | !order) {
    printf("Malloc failed for RAB setup.\n");
    return -ENOMEM;
  }
  j=0;
  for (i=0;i<n_data;i++) {
    if ( (*data_idxs)[i] == 1 ) {
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
  use_acp_int[0] = task->data_desc[order[0]].use_acp;
  rab_lvl_int[0] = task->data_desc[order[0]].rab_lvl;
  for (i=1;i<n_idxs;i++) {
    j = order[i];
    gap_size = (unsigned)task->data_desc[j].ptr - (v_addr_int[n_data_int-1]
                                                   + size_int[n_data_int-1]);
    // !!!!TO DO: check protections, check dates!!!
    if ( gap_size > RAB_CONFIG_MAX_GAP_SIZE_B
         || task->data_desc[j].use_acp != use_acp_int[n_data_int-1]
         || task->data_desc[j].rab_lvl != rab_lvl_int[n_data_int-1]) { 
      // the gap is too large or different ACP setting is used or different RAB level is requested, create a new mapping
      n_data_int++;
      v_addr_int[n_data_int-1] = (unsigned)task->data_desc[j].ptr;
      size_int[n_data_int-1] = (unsigned)task->data_desc[j].size;
      use_acp_int[n_data_int-1] = task->data_desc[j].use_acp;
      rab_lvl_int[n_data_int-1] = task->data_desc[j].rab_lvl;
    }
    else {
      // extend the previous mapping
      size_int[n_data_int-1] += (gap_size + task->data_desc[j].size);  
    }
  }
  // set up the RAB
  if (DEBUG_LEVEL > 2) {
    printf("Requesting %d remapping(s):\n",n_data_int);
  }
  for (i=0;i<n_data_int;i++) {
    if (DEBUG_LEVEL > 2) {
      printf("%d \t %#x \t %#x \n",i,v_addr_int[i],size_int[i]);
      usleep(1000000);
    }
    pulp_rab_req(pulp, v_addr_int[i], size_int[i], prot, port, date_exp, date_cur, use_acp_int[i], rab_lvl_int[i]);
  }

  // set up RAB stripes
  //pulp_rab_req_striped(pulp, task, data_idxs, n_idxs, prot, port);
  if ( !strcmp(task->name, "profile_rab_striping") ) {
    pulp_rab_req_striped(pulp, task, data_idxs, task->n_data);
  }
  else if ( !strcmp(task->name, "rod") ) {
    pulp_rab_req_striped(pulp, task, data_idxs, 3);
  }
  else if ( !strcmp(task->name, "ct") ) {
    pulp_rab_req_striped(pulp, task, data_idxs, 1);
  }
  else if ( !strcmp(task->name, "jpeg") ) {
    pulp_rab_req_striped(pulp, task, data_idxs, 1);
  }
  else {
    if ( strcmp(task->name, "face_detect") )
      printf("ERROR: Unknown task name %s\n",task->name);
  }
  // free memory
  free(v_addr_int);
  free(size_int);
  free(use_acp_int);
  free(rab_lvl_int);
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
  int i;
  unsigned n_data;

  n_data = task->n_data;

  if (DEBUG_LEVEL > 2) {
    printf("Mailbox status = %#x.\n",pulp_read32(pulp->mbox.v_addr, MBOX_STATUS_OFFSET_B, 'b'));
  }

  for (i=0;i<n_data;i++) {

    if ( (*data_idxs)[i] ) {
      // pass data element by reference
      pulp_mbox_write(pulp, (unsigned)(task->data_desc[i].ptr));
      if (DEBUG_LEVEL > 2)
        printf("Element %d: wrote %#x to mbox.\n",i,(unsigned) (task->data_desc[i].ptr));
    }
    else {
      // pass data element by value
      pulp_mbox_write(pulp, *(unsigned *)(task->data_desc[i].ptr));
      if (DEBUG_LEVEL > 2)
        printf("Element %d: wrote %#x to mbox.\n",i,*(unsigned*)(task->data_desc[i].ptr));
    }    
  }
  
  if (DEBUG_LEVEL > 1) {
    printf("Passed %d of %d data elements to PULP.\n",i,n_data);
  }

  return i;
}

/**
 * Get back the shared data elements from PULP that were passed by value.
 *
 * @pulp:      pointer to the PulpDev structure
 * @task:      pointer to the TaskDesc structure
 * @data_idxs: pointer to array marking the elements to pass by reference
 * @n_idxs:    number of shared data elements passed by reference
 */
int pulp_offload_get_desc(PulpDev *pulp, TaskDesc *task, unsigned **data_idxs, int n_idxs)
{
  int i,j, n_data;
  unsigned *buffer;

  j = 0;
  n_data = task->n_data;

  buffer = (unsigned *)malloc((n_data-n_idxs)*sizeof(unsigned));
  if ( buffer == NULL ) {
    printf("ERROR: Malloc failed for buffer.\n");
    return -ENOMEM;
  }
  
  // read from mbox
  pulp_mbox_read(pulp, &buffer[0], n_data-n_idxs);
  
  for (i=0; i<n_data; i++) {
    // check if argument has been passed by value
    if ( (*data_idxs)[i] == 0 ) {
      // read from buffer
      *(unsigned *)(task->data_desc[i].ptr) = buffer[j];
      j++;
    }
  }
  
  //for (i=0; i<n_data; i++) {
  //  // check if argument has been passed by value
  //  if ( (*data_idxs)[i] == 0 ) {
  //    // read from mbox
  //    pulp_mbox_read(pulp, task->data_desc[i].ptr, 1);
  //    j++;
  //  }
  //}
  
  if (DEBUG_LEVEL > 1) {
    printf("Got back %d of %d data elements from PULP.\n",j,n_data-n_idxs);
  }

#ifndef JPEG // generates error
  free(buffer);
#endif

  return j;
}

/**
 * Offload a new task to PULP, set up RAB slices and pass descriptors
 * to PULP. 
 *
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
  
#if (MEM_SHARING != 1)
  // RAB setup
  err = pulp_offload_rab_setup(pulp, task, &data_idxs, n_idxs);
  if (err) {
    printf("ERROR: pulp_offload_rab_setup failed.\n");
    return err;
  }
#endif

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
 *
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

  // read back data elements with sizes up to 32 bit from mbox
  n_idxs = pulp_offload_get_data_idxs(task, &data_idxs);
 
#if (MEM_SHARING != 3) 
  // free RAB slices
  date_cur = (unsigned char)(task->task_id + 4);
  pulp_rab_free(pulp, date_cur);

  if ( !strcmp(task->name, "profile_rab_striping") || !strcmp(task->name, "rod")
       || !strcmp(task->name, "ct") || !strcmp(task->name, "jpeg") ) {
    // free striped RAB slices
    pulp_rab_free_striped(pulp);   
  }
#endif

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
 *
 * @pulp: pointer to the PulpDev structure 
 * @task: pointer to the TaskDesc structure
 */
int pulp_offload_start(PulpDev *pulp, TaskDesc *task)
{
  unsigned status;

  if (DEBUG_LEVEL > 2) {
    printf("Mailbox status = %#x.\n",pulp_read32(pulp->mbox.v_addr, MBOX_STATUS_OFFSET_B, 'b'));
  }
  
  // read status
  pulp_mbox_read(pulp, &status, 1);
  if ( status != PULP_READY ) {
    printf("ERROR: PULP status not ready. PULP status = %#x.\n",status);
    return -EBUSY;
  }

  // start execution
  pulp_mbox_write(pulp, PULP_START);

  return 0;
}

/**
 * Wait for an offloaded task to finish on PULP.
 *
 * @pulp: pointer to the PulpDev structure 
 * @task: pointer to the TaskDesc structure
 */
int pulp_offload_wait(PulpDev *pulp, TaskDesc *task)
{
  unsigned status;

  // check if sync = 0x1
  status = 0;
  while ( status != PULP_DONE ) {
    status = pulp_read32(pulp->l2_mem.v_addr, SYNC_OFFSET_B,'b');
  }

  // read status
  pulp_mbox_read(pulp, &status, 1);
  if ( status != PULP_DONE ) {
    printf("ERROR: PULP status not done. PULP status = %#x.\n",status);
    return -EBUSY;
  }

  return 0;
}

//
int pulp_offload_out_contiguous(PulpDev *pulp, TaskDesc *task, TaskDesc **ftask)
{
  // similar to pulp_offload_out() but without RAB setup
  int err;
  unsigned *data_idxs;
  data_idxs = (unsigned *)malloc(task->n_data*sizeof(unsigned));
  if ( data_idxs == NULL ) {
    printf("ERROR: Malloc failed for data_idxs.\n");
    return -ENOMEM;
  }

  // only remap addresses belonging to data elements larger than 32 bit  
  pulp_offload_get_data_idxs(task, &data_idxs);

  int i;
  int data_size, data_ptr, data_type;

  *ftask = (TaskDesc *)malloc(sizeof(TaskDesc));
  if ( *ftask == NULL ) {
    printf("ERROR: Malloc failed for ftask.\n");
    return -ENOMEM;
  }

  (*ftask)->task_id = task->task_id;
  (*ftask)->name = task->name;
  (*ftask)->n_clusters = task->n_clusters;
  (*ftask)->n_data = task->n_data;

  if ( !strcmp(task->name, "rod") ) {
#ifdef PROFILE   
    (*ftask)->data_desc  = (DataDesc *)malloc(9*sizeof(DataDesc));
#else
    (*ftask)->data_desc  = (DataDesc *)malloc(6*sizeof(DataDesc));
#endif // PROFILE    
  }
  else if ( !strcmp(task->name, "ct") ) {
#ifdef PROFILE   
    (*ftask)->data_desc  = (DataDesc *)malloc(8*sizeof(DataDesc));
#else
    (*ftask)->data_desc  = (DataDesc *)malloc(6*sizeof(DataDesc));
#endif // PROFILE    
  }
  else if ( !strcmp(task->name, "jpeg") ) {
#ifdef PROFILE   
    (*ftask)->data_desc  = (DataDesc *)malloc(10*sizeof(DataDesc));
#else
    (*ftask)->data_desc  = (DataDesc *)malloc(7*sizeof(DataDesc));
#endif // PROFILE    
  }
  else {
    printf("ERROR: Unknown task name %s\n",task->name);
  }

  if ( ((*ftask)->data_desc) == NULL ) {
    printf("ERROR: Malloc failed for data_desc.\n");
    return -ENOMEM;
  }
 
  // memory allocation in contiguous L3 memory for IN and OUT
  // copy to contiguous L3 memory for IN 
  for (i = 0; i < (*ftask)->n_data; i++) {
    
    data_size = task->data_desc[i].size;
    data_ptr  = (int)task->data_desc[i].ptr;

    // data element to pass by reference, allocate contiguous L3 memory
    if (data_idxs[i] > 0) {
                  
      data_type = task->data_desc[i].type;
     
      // we are going to abuse type to store the virtual host address and
      // ptr to store the physical address in contiguous L3 used by PULP

      switch(data_type) {
      case 0:
        //INOUT
        (*ftask)->data_desc[i].type = (int)pulp_l3_malloc(pulp, data_size,
                                                          (unsigned *)&((*ftask)->data_desc[i].ptr));
        memcpy((void *)(*ftask)->data_desc[i].type, (void *)data_ptr ,data_size);
        break;
                    
      case 1:
        //IN
        (*ftask)->data_desc[i].type = (int)pulp_l3_malloc(pulp, data_size,
                                                          (unsigned *)&((*ftask)->data_desc[i].ptr));
        memcpy((void *)(*ftask)->data_desc[i].type, (void *)data_ptr ,data_size);
        break;
                    
      case 2:
        //OUT
        (*ftask)->data_desc[i].type = (int)pulp_l3_malloc(pulp, data_size,
                                                          (unsigned *)&((*ftask)->data_desc[i].ptr));
        break;
                    
      default:
        //NONE
        break;
      }
    }
    
    // data element to pass by value (just copy descriptor)
    else  {
      (*ftask)->data_desc[i].ptr = (void *)data_ptr;
    }
    (*ftask)->data_desc[i].size = data_size;
  }

  // Pass data descriptor to PULP
  err = pulp_offload_pass_desc(pulp, *ftask, &data_idxs);
  if ( err != task->n_data ) {
    printf("ERROR: pulp_offload_pass_desc failed.\n");
    return err;
  }

  // free memory
  free(data_idxs);
  
  return 0;
}

int pulp_offload_in_contiguous(PulpDev *pulp, TaskDesc *task, TaskDesc **ftask)
{
  int i;

  // similar to pulp_offload_in() but without RAB stuff 
  int err, n_idxs;
  unsigned *data_idxs;
  data_idxs = (unsigned *)malloc(task->n_data*sizeof(unsigned));
  if ( data_idxs == NULL ) {
    printf("ERROR: Malloc failed for data_idxs.\n");
    return -ENOMEM;
  }

  // read back data elements with sizes up to 32 bit from mbox
  n_idxs = pulp_offload_get_data_idxs(task, &data_idxs);

  // fetch values of data elements passed by value
  err = pulp_offload_get_desc(pulp, task, &data_idxs, n_idxs);
  if ( err != (task->n_data - n_idxs) ) {
    printf("ERROR: pulp_offload_get_desc failed.\n");
    return err;
  }

  // copy back result to virtual paged memory
  for (i = 0; i < (*ftask)->n_data; i++) {

    //if not passed by value 
    if (data_idxs[i] > 0) {
  
      // we are abusing type of ftask->dta_desc  to store the virtual host address and
      // ptr to store the physical address in contiguous L3 used by PULP

      switch(task->data_desc[i].type) {
      case 0:
        //INOUT
        memcpy((void *)task->data_desc[i].ptr, (void *)(*ftask)->data_desc[i].type, task->data_desc[i].size);
        pulp_l3_free(pulp, (unsigned)(*ftask)->data_desc[i].type, (unsigned)(*ftask)->data_desc[i].ptr);
        break;
                    
      case 1:
        //IN                    
        pulp_l3_free(pulp, (unsigned)(*ftask)->data_desc[i].type, (unsigned)(*ftask)->data_desc[i].ptr);
        break;

      case 2:
        //OUT
        memcpy((void *)task->data_desc[i].ptr, (void *)(*ftask)->data_desc[i].type, task->data_desc[i].size);
        pulp_l3_free(pulp, (unsigned)(*ftask)->data_desc[i].type, (unsigned)(*ftask)->data_desc[i].ptr);
        break;
                    
      default:
        //NONE
        break;
      }
    }
  }

  // free memory
  free((*ftask)->data_desc);
  free(*ftask);
  free(data_idxs);

  return 0;
}

/****************************************************************************************/

int pulp_rab_req_striped_mchan_img(PulpDev *pulp, unsigned char prot, unsigned char port,
                                   unsigned p_height, unsigned p_width, unsigned i_step,
                                   unsigned n_channels, unsigned char **channels,
                                   unsigned *s_height)
{
  int i,j;

  RabStripeReqUser stripe_request;

  stripe_request.id         = 0;
  stripe_request.n_elements = 1;

  RabStripeElemUser * elements = (RabStripeElemUser *)
    malloc((size_t)(stripe_request.n_elements*sizeof(RabStripeElemUser)));
  if ( elements == NULL ) {
    printf("ERROR: Malloc failed for RabStripeElemUser structs.\n");
    return -ENOMEM;
  }

  stripe_request.rab_stripe_elem_user_addr = (unsigned)&elements[0];

  unsigned * addr_start;
  unsigned * addr_end;

  unsigned addr_start_tmp, addr_end_tmp;
  unsigned stripe_size_b, stripe_height, n_stripes_per_channel;

  unsigned page_size_b;

  page_size_b = getpagesize();

  // compute max stripe height
  stripe_size_b = (RAB_L1_N_SLICES_PORT_1/2-1)*page_size_b;
  stripe_height = stripe_size_b / i_step;    
  
  n_stripes_per_channel = p_height / stripe_height;
  if (p_height % stripe_height)
    n_stripes_per_channel++;

  // compute effective stripe height
  stripe_height = p_height / n_stripes_per_channel;
  *s_height = stripe_height;
  stripe_size_b = stripe_height * i_step;
     
  if (DEBUG_LEVEL > 2) {
    printf("n_stripes_per_channel = %d\n", n_stripes_per_channel);
    printf("stripe_size_b = %#x\n", stripe_size_b);
    printf("s_height = %d\n", stripe_height);
    printf("page_size_b = %#x\n",page_size_b);
  }

  // generate the rab_stripes table
  elements[0].id    = 0;   // not used right now
  elements[0].type  = 2;   // for now in
  elements[0].flags = 0x7;
  elements[0].max_stripe_size_b = stripe_size_b;
  elements[0].n_stripes = n_stripes_per_channel * n_channels;

  addr_start = (unsigned *)malloc((size_t)elements[0].n_stripes*sizeof(unsigned));
  addr_end   = (unsigned *)malloc((size_t)elements[0].n_stripes*sizeof(unsigned));
  if ( (addr_start == NULL) || (addr_end == NULL) ) {
    printf("ERROR: Malloc failed for addr_start/addr_end.\n");
    return -ENOMEM;
  }

  elements[0].stripe_addr_start = (unsigned)&addr_start[0];
  elements[0].stripe_addr_end   = (unsigned)&addr_end[0];

  // fill in stripe data
  for (i=0; i<n_channels; i++) {
    for (j=0; j<n_stripes_per_channel; j++) {
      // align to words
      addr_start_tmp = ((unsigned)channels[i] + stripe_size_b*j);
      BF_SET(addr_start_tmp,0,0,4);
      addr_end_tmp   = ((unsigned)channels[i] + stripe_size_b*(j+1));
      BF_SET(addr_end_tmp,0,0,4);
      addr_end_tmp += 0x4;

      addr_start[i*n_stripes_per_channel + j] = addr_start_tmp;
      addr_end  [i*n_stripes_per_channel + j] = addr_end_tmp;
    }
  }

  if (DEBUG_LEVEL > 2) {
    printf("Shared Element %d: \n",0);
    printf("stripe_addr_start @ %#x\n",elements[0].stripe_addr_start);
    printf("stripe_addr_end   @ %#x\n",elements[0].stripe_addr_end);
    for (j=0; j<elements[0].n_stripes; j++) {
      if (j>2 && j<(elements[0].n_stripes-3))
        continue;
      printf("%d\t",j);
      printf("%#x - ", ((unsigned *)(elements[0].stripe_addr_start))[j]);
      printf("%#x\n",  ((unsigned *)(elements[0].stripe_addr_end))[j]);
      printf("\n");
    }
  }

  // write the img to a file
  //#define PATCH2FILE
  //#define IMG2FILE
#if defined(IMG2FILE) || defined(PATCH2FILE) 
  
  FILE *fp;
  
  // open the file  
  if((fp = fopen("img.h", "a")) == NULL) {
    printf("ERROR: Could not open img.h.\n");
    return -ENOENT;
  }

  printf("p_width  = %d\n",p_width);
  printf("p_height = %d\n",p_height);
  printf("i_step   = %d\n",i_step);
 
#if defined(PATCH2FILE)

  //write start
  printf(fp, "unsigned char img[%d] = {\n",p_width * p_height * n_channels);

  for (i=0; i<n_channels; i++) {
    fprintf(fp,"// Channel %d: \n",i);
    for (j=0; j<p_height; j++) {
      for (k=0; k<p_width; k++) {
        fprintf(fp,"\t%#x,",(unsigned) *(channels[i]+j*i_step+k));
      }
      fprintf(fp,"\n");
    }
  }

#elif defined(IMG2FILE)

  unsigned i_height, i_width;
  i_height = 21;
  i_width = 37;

  // write start
  fprintf(fp, "unsigned char img[%d] = {\n",i_height * i_step * n_channels);

  for (i=0; i<n_channels; i++) {
    //if ((i == 0) || (i == 16)) {
    fprintf(fp,"// Channel %d: \n",i);
    for (j=0; j<i_height; j++) {
      for (k=0; k<i_step; k++) {
        fprintf(fp,"\t%#x,",(unsigned) *(channels[i]+j*i_step+k));
      }
      fprintf(fp,"\n");
    }
    //}
  }

  // write end
  fprintf(fp, "};\n\n");
#endif  

  fclose(fp);
#endif

  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_REQ_STRIPED,(unsigned *)&stripe_request);

  // free memory
  free((unsigned *)elements[0].stripe_addr_start);
  free((unsigned *)elements[0].stripe_addr_end);
  free(elements);

  return 0;
}
