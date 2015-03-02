#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

#define DUMMY_SIZE_B 0x1000
#define DUMMY_SIZE   DUMMY_SIZE_B/4

#define PULP_FREQ_MHZ 100

int main(){
  
  __attribute__((aligned(64))) int dummy[DUMMY_SIZE];

  /*
   * Initialization
   */ 
  // global variables
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // initialization of pulp
  pulp_reserve_v_addr(pulp);
  pulp_mmap(pulp);
  //sleep(1);
  //pulp_print_v_addr(pulp);
  //sleep(1);  
  pulp_reset(pulp);
  pulp_clking_set_freq(pulp,PULP_FREQ_MHZ);
  pulp_rab_free(pulp,0x0);
  pulp_init(pulp);
  
  /*
   * Body
   */
  int i;
  unsigned l2_offset;
  
  l2_offset = 0x0;

  // initialize the arrays
  for(i=0; i<DUMMY_SIZE; i++) {
    dummy[i] = i+1;
  }
 
  /*
   * Run a test program on PULP to measure the latencies
   */
  printf("Testing PULP memory latencies...\n");  
  
  // manually setup data structures (no compiler support right now)
  TaskDesc task_desc;
   
  char name[30];
  strcpy(name,"latency_measurement");
  
  task_desc.name = &name[0];
  task_desc.n_clusters = -1;
  task_desc.n_data = 1;
   
  DataDesc data_desc[task_desc.n_data];
  data_desc[0].ptr = (unsigned *)&dummy;
  data_desc[0].size   = DUMMY_SIZE_B;
  
  task_desc.data_desc = data_desc;
  
  sleep(1);
  
  // issue the offload
  pulp_omp_offload_task(pulp,&task_desc);
  //pulp_omp_wait(pulp,ker_id);
  
  sleep(1);

  int time_high, time_low;
  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');

  float time, cycles;
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/PULP_FREQ_MHZ;
  printf("PULP - L1: read latency = %.2f [cycles]\n",cycles);
  printf("PULP - L1: read latency = %.2f [us]\n",time);

  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/PULP_FREQ_MHZ;
  printf("PULP - L1: write latency = %.2f [cycles]\n",cycles);
  printf("PULP - L1: write latency = %.2f [us]\n",time);

  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/PULP_FREQ_MHZ;
  printf("PULP - L2: read latency = %.2f [cycles]\n",cycles);
  printf("PULP - L2: read latency = %.2f [us]\n",time);

  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/PULP_FREQ_MHZ;
  printf("PULP - L2: write latency = %.2f [cycles]\n",cycles);
  printf("PULP - L2: write latency = %.2f [us]\n",time);

  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/PULP_FREQ_MHZ;
  printf("PULP - L3: read latency = %.2f [cycles]\n",cycles);
  printf("PULP - L3: read latency = %.2f [us]\n",time);

  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/PULP_FREQ_MHZ;
  printf("PULP - L3: write latency = %.2f [cycles]\n",cycles);
  printf("PULP - L3: write latency = %.2f [us]\n",time);

  //pulp_stdout_print(pulp,0);
  //pulp_stdout_print(pulp,1);
  //pulp_stdout_print(pulp,2);
  //pulp_stdout_print(pulp,3);
  //pulp_stdout_clear(pulp,0);
  //pulp_stdout_clear(pulp,1);
  //pulp_stdout_clear(pulp,2);
  //pulp_stdout_clear(pulp,3);

  sleep(3);

  /*
   * Cleanup
   */
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
