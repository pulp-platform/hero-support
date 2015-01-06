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

  printf("Freeing reserved virtual addresses overlapping with physical address map of PULP.\n");

  status = munmap(pulp->reserved_v_addr.v_addr,pulp->reserved_v_addr.size);
  if (status) {
    printf("MUNMAP failed to free reserved virtual addresses overlapping with physical address map of PULP.\n");
  } 

  return 0;
}

/*
 * Print information about the reserved virtual memory on the host
 */
void pulp_print_v_addr(PulpDev *pulp) {

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
  // PULP internals
  // Clusters
  offset = 0; // start of clusters
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
  offset = CLUSTERS_SIZE_B; // start of peripherals
  pulp->soc_periph.size = SOC_PERIPHERALS_SIZE_B;
 
  pulp->soc_periph.v_addr = mmap(NULL,pulp->soc_periph.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->soc_periph.v_addr == MAP_FAILED) {
    printf("MMAP failed for SoC peripherals.\n");
    return EIO;
  }
  else {
    printf("SoC peripherals mapped to virtual user space at %p.\n",pulp->soc_periph.v_addr);
  }

  // Mailbox
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B; // start of mailbox
  pulp->mailbox.size = MAILBOX_SIZE_B;
 
  pulp->mailbox.v_addr = mmap(NULL,pulp->mailbox.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset); 
  if (pulp->mailbox.v_addr == MAP_FAILED) {
    printf("MMAP failed for Mailbox.\n");
    return EIO;
  }
  else {
    printf("SoC peripherals mapped to virtual user space at %p.\n",pulp->mailbox.v_addr);
  }
 
  // L2
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B; // start of L2
  pulp->l2_mem.size = L2_MEM_SIZE_B;
 
  pulp->l2_mem.v_addr = mmap(NULL,pulp->l2_mem.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
 
  if (pulp->l2_mem.v_addr == MAP_FAILED) {
    printf("MMAP failed for L2 memory.\n");
    return EIO;
  }
  else {
     printf("L2 memory mapped to virtual user space at %p.\n",pulp->l2_mem.v_addr);
  }

  // Platform
  // L3
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B; // start of L3
  pulp->l3_mem.size = L3_MEM_SIZE_B;
    
  pulp->l3_mem.v_addr = mmap(NULL,pulp->l3_mem.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->l3_mem.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("Shared L3 memory mapped to virtual user space at %p.\n",pulp->l3_mem.v_addr);
  }
 
  // PULP external
  // GPIO
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B; // start of GPIO
  pulp->gpio.size = H_GPIO_SIZE_B;
    
  pulp->gpio.v_addr = mmap(NULL,pulp->gpio.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->gpio.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("GPIO memory mapped to virtual user space at %p.\n",pulp->gpio.v_addr);
  }

  // CLKING
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B; // start of Clking
  pulp->clking.size = CLKING_SIZE_B;
    
  pulp->clking.v_addr = mmap(NULL,pulp->clking.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->clking.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("Clock Manager memory mapped to virtual user space at %p.\n",pulp->clking.v_addr);
  }

  // STDOUT
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B; // start of Stdout
  pulp->stdout.size = STDOUT_SIZE_B;
    
  pulp->stdout.v_addr = mmap(NULL,pulp->stdout.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->stdout.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("Stdout memory mapped to virtual user space at %p.\n",pulp->stdout.v_addr);
  }

  // RAB config
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B; // start of RAB config
  pulp->rab_config.size = RAB_CONFIG_SIZE_B;
    
  pulp->rab_config.v_addr = mmap(NULL,pulp->rab_config.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->rab_config.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("RAB config memory mapped to virtual user space at %p.\n",pulp->rab_config.v_addr);
  }

  // Zynq
  // SLCR
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B; // start of SLCR
  pulp->slcr.size = SLCR_SIZE_B;
    
  pulp->slcr.v_addr = mmap(NULL,pulp->slcr.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->slcr.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("Zynq SLCR memory mapped to virtual user space at %p.\n",pulp->slcr.v_addr);
  }

  // MPCore
  offset = CLUSTERS_SIZE_B + SOC_PERIPHERALS_SIZE_B + MAILBOX_SIZE_B + L2_MEM_SIZE_B + L3_MEM_SIZE_B + H_GPIO_SIZE_B + CLKING_SIZE_B + STDOUT_SIZE_B + RAB_CONFIG_SIZE_B + SLCR_SIZE_B; // start of MPCore
  pulp->mpcore.size = MPCORE_SIZE_B;
    
  pulp->mpcore.v_addr = mmap(NULL,pulp->mpcore.size,PROT_READ | PROT_WRITE,MAP_SHARED,pulp->fd,offset);
  if (pulp->mpcore.v_addr == MAP_FAILED) {
    printf("MMAP failed for shared L3 memory.\n");
    return EIO;
  }
  else {
    printf("Zynq MPCore memory mapped to virtual user space at %p.\n",pulp->mpcore.v_addr);
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

  // set fetch enable to 0, disable reset
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x80000000);

  // RAB setup
  // port 0: Host -> PULP
  pulp_rab_req(pulp,L2_MEM_H_BASE_ADDR,L2_MEM_SIZE_B,0x7,0,0xFF,0xFF);   // L2
  //pulp_rab_req(pulp,MAILBOX_H_BASE_ADDR,MAILBOX_SIZE_B,0x7,0,0xFF,0xFF); // Mailbox, Interface 0
  pulp_rab_req(pulp,MAILBOX_H_BASE_ADDR,MAILBOX_SIZE_B*2,0x7,0,0xFF,0xFF); // Mailbox, Interface 0 and Interface 1
  pulp_rab_req(pulp,PULP_H_BASE_ADDR,0x10000,0x7,0,0xFF,0xFF); // TCDM
  // port 1: PULP -> Host
  pulp_rab_req(pulp,L3_MEM_BASE_ADDR,L3_MEM_SIZE_B,0x7,1,0xFF,0xFF);     // L3 memory (contiguous)
  
  // enable mailbox interrupts
  pulp_write32(pulp->mailbox.v_addr,MAILBOX_IE_OFFSET_B,'b',0x6);
 
  return 0;
}

int pulp_rab_req(PulpDev *pulp, unsigned addr_start, unsigned size_b, unsigned char prot, unsigned char port, unsigned char date_exp, unsigned char date_cur) {
  
  unsigned request[3];

  // setup the request
  request[0] = 0;
  BF_SET(request[0], date_cur, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT + RAB_CONFIG_N_BITS_DATE, RAB_CONFIG_N_BITS_DATE);
  BF_SET(request[0], date_exp, RAB_CONFIG_N_BITS_PROT + RAB_CONFIG_N_BITS_PORT, RAB_CONFIG_N_BITS_DATE);
  BF_SET(request[0], port, RAB_CONFIG_N_BITS_PROT, RAB_CONFIG_N_BITS_PORT);
  BF_SET(request[0], prot, 0, RAB_CONFIG_N_BITS_PROT);
  request[1] = addr_start;
  request[2] = size_b;
  
  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_REQ,request);
  
  return 0;
} 

void pulp_rab_free(PulpDev *pulp, unsigned char date_cur) {
  
  // make the request
  ioctl(pulp->fd,PULP_IOCTL_RAB_FREE,(unsigned)date_cur);
  
  return;
} 


int pulp_omp_offload_task(PulpDev *pulp, TaskDesc *task) {

  int i,j,temp;
 
  /*
   * RAB setup
   */
  unsigned char prot, port, date_exp, date_cur;

  prot = 0x7;
  port = 1;  // PULP -> Host
  date_exp = 0x1;
  date_cur = 0x1; // Overwrite old mappings
  
  // check if the shared data are located in the same memory page
  unsigned n_data, n_data_int, gap_size;
  unsigned * v_addr_int;
  unsigned * size_int;
  unsigned * order;

  n_data = task->n_data;
  n_data_int = 1;

  v_addr_int = (unsigned *)malloc((size_t)n_data*sizeof(unsigned));
  size_int = (unsigned *)malloc((size_t)n_data*sizeof(unsigned));
  order = (unsigned *)malloc((size_t)n_data*sizeof(unsigned));
  if (!v_addr_int | !size_int | !order) {
    printf("Malloc failed for RAB setup\n");
    return -ENOMEM;
  }
  for (i=0;i<n_data;i++)
    order[i] = i;

  // order the elements - bubble sort
  for (i=n_data;i>1;i--) {
    for (j=0;j<i-1;j++) {
      if (task->data_desc[j].v_addr > task->data_desc[j+1].v_addr) {
	temp = order[j];
	order[j] = order[j+1];
	order[j+1] = temp;
      }
    }
  }
  // for (i=0;i<n_data;i++) {
  //   printf("%d \t %#x \t %#x \n",order[i], (unsigned)task->data_desc[order[i]].v_addr, (unsigned)task->data_desc[order[i]].size);
  // }

  v_addr_int[0] = (unsigned)task->data_desc[order[0]].v_addr; 
  size_int[0] = (unsigned)task->data_desc[order[0]].size; 
  for (i=1;i<n_data;i++) {
    j = order[i];
    gap_size = (unsigned)task->data_desc[j].v_addr - (v_addr_int[n_data_int-1] + size_int[n_data_int-1]);
    if ( gap_size > RAB_CONFIG_MAX_GAP_SIZE_B ) { // to do: check protections, check dates!!!
      // the gap is too large, create a new mapping
      n_data_int++;
      v_addr_int[n_data_int-1] = (unsigned)task->data_desc[j].v_addr;
      size_int[n_data_int-1] = (unsigned)task->data_desc[j].size;
    }
    else {
      // extend the previous mapping
      size_int[n_data_int-1] += (gap_size + task->data_desc[j].size);  
    }
  }

  // setup the RAB
  for (i=0;i<n_data_int;i++) {
    pulp_rab_req(pulp, v_addr_int[i], size_int[i], prot, port, date_exp, date_cur);
  }

  free(v_addr_int);
  free(size_int);
  free(order);
  
  /*
   * write virtual addresses to mailbox
   */
  for (i=0;i<n_data;i++) {
    pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',(unsigned) (task->data_desc[i].v_addr));
  }

  // check the virtual addresses
  for (i=0;i<task->n_data;i++) {
    printf("v_addr_%d %#x\n",i,(unsigned) (task->data_desc[i].v_addr));
  }


  /*
   * offload
   */
  // for now: simple binary offload
  // prepare binary
  char * bin_name;
  bin_name = (char *)malloc((strlen(task->name)+4)*sizeof(char));
  if (!bin_name) {
    printf("Malloc failed for bin_name\n");
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
  
  // reverse endianness
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

int pulp_check_results(PulpDev *pulp) {

   return 0;
};

void pulp_reset(PulpDev *pulp) {

  unsigned slcr_value;

  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x00000000);
  usleep(10);
  pulp_write32(pulp->gpio.v_addr,0x8,'b',0x80000000);

  // FPGA reset control register
  slcr_value = pulp_read32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b');
  
  // Extract the FPGA_OUT_RST bits
  slcr_value = slcr_value & 0xF;
  
  // Enable reset
  pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', slcr_value | (0x1 << SLCR_FPGA_OUT_RST));
    
  // Wait
  usleep(10);
    
  // Disable reset
  pulp_write32(pulp->slcr.v_addr, SLCR_FPGA_RST_CTRL_OFFSET_B, 'b', slcr_value & (0xF & (0x0 << SLCR_FPGA_OUT_RST)));
}

  // release PL reset
  //pulp_write32(pulp->gpio.v_addr,0x8,'b',0);
  //usleep(10);
  //pulp_write32(pulp->gpio.v_addr,0x8,'b',3);

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
  //offset = 0x10*((RAB_N_PORTS-2)*RAB_N_SLICES+0);
  //printf("RAB config offset = %#x\n",offset);
  //pulp_write32(pulp->rab_config.v_addr,offset+0x10,'b',PULP_H_BASE_ADDR);
  //pulp_write32(pulp->rab_config.v_addr,offset+0x14,'b',PULP_H_BASE_ADDR+L2_MEM_SIZE_B);
  //pulp_write32(pulp->rab_config.v_addr,offset+0x18,'b',L2_MEM_BASE_ADDR);
  //pulp_write32(pulp->rab_config.v_addr,offset+0x1c,'b',0x7);

