#include <unistd.h> // for sleep
#include <stdlib.h> // for system

#include "../../lib/inc/zynq.h"
#include "../../lib/inc/pulp.h"
#include "../../lib/inc/pulp_func.h"

int main(){
  
  // global variables
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  pulp_reserve_v_addr(pulp);
  
  sleep(1);
  
  ///////////////////////////////////////////////////////////////////////////
  
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
  
  ///////////////////////////////////////////////////////////////////////////
  
  sleep(1);

  pulp_mmap(pulp);
  
  pulp_reset(pulp);

  pulp_init(pulp);
  
  pulp_offload_kernel(pulp, true);

  pulp_check_results(pulp);
  
//
  //// Reread from TCDM
  //printf("TCDM - 1: %#x \n",pulp_read32(pulp->clusters.v_addr,0x5000,'b'));
  //printf("TCDM - 2: %#x \n",pulp_read32(pulp->clusters.v_addr,0x5004,'b'));
  //printf("TCDM - 3: %#x \n",pulp_read32(pulp->clusters.v_addr,0x5008,'b'));

  //// DMA transfers
  //printf("Setup DMA transfer from L2 to L3.\n");
  //pulp_dma_transfer(pulp,0x2C000000,0x18020000,0x100);
  //sleep(2);
  //
  //printf("L3_2 - 1: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x20000,'b'));
  //printf("L3_2 - 2: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x20004,'b'));
  //printf("L3_2 - 3: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x20008,'b'));
  //
  //printf("Setup DMA transfer from TCDM to L3.\n");
  //pulp_dma_transfer(pulp,0x28005000,0x18030000,0x100);
  //sleep(2);
  //
  //printf("L3_3 - 1: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x30000,'b'));
  //printf("L3_3 - 2: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x30004,'b'));
  //printf("L3_3 - 3: %#x \n",pulp_read32(pulp->l3_mem.v_addr,0x30008,'b'));
  //
  //printf("Setup DMA transfer from L3 to TCDM.\n");
  //pulp_dma_transfer(pulp,0x18010000,0x20006000,0x100);
  //sleep(2);
  //
  //printf("TCDM_2 - 1: %#x \n",pulp_read32(pulp->clusters.v_addr,0x6000,'b'));
  //printf("TCDM_2 - 2: %#x \n",pulp_read32(pulp->clusters.v_addr,0x6004,'b'));
  //printf("TCDM_2 - 3: %#x \n",pulp_read32(pulp->clusters.v_addr,0x6008,'b'));

  sleep(1);
  pulp_munmap(pulp);
  pulp_free_v_addr(pulp);

  return 0;
}
