#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

//#define USE_L3
#define CHECK_RESULT
#define PRINT_MAT

#define ARM_DMA_SIZE_B 0x10000 // 64kB = L2 Size
//#define ARM_DMA_SIZE_B 0x8000
//#define ARM_DMA_SIZE_B 0x1000
#define PULP_DMA_SIZE_B 0x2000 // 8kB

#define ARM_DMA_SIZE  ARM_DMA_SIZE_B/4
#define PULP_DMA_SIZE  PULP_DMA_SIZE_B/4

int main(){
  
  //// preparation
  //int * arm_dma_array;
  //int * pulp_dma_array;
  //
  //if (posix_memalign((void **)&arm_dma_array,(size_t)64,(size_t)sizeof(int)*ARM_DMA_SIZE))
  //  printf("Error: memory allocation failed.\n");
  //if (posix_memalign((void **)&pulp_dma_array,(size_t)64,(size_t)sizeof(int)*PULP_DMA_SIZE))
  //  printf("Error: memory allocation failed.\n");

  __attribute__((aligned(64))) int arm_dma_array[ARM_DMA_SIZE];
  __attribute__((aligned(64))) int pulp_dma_array[PULP_DMA_SIZE];

  // setup time measurement
  struct timespec res, tp1, tp2;
  int s_duration;
  long ns_duration;

  clock_getres(CLOCK_REALTIME,&res);

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
  //pulp_clking_set_freq(pulp,100);
  pulp_rab_free(pulp,0x0);
  pulp_init(pulp);
  
  /*
   * Body
   */
  int i;
  unsigned l2_offset;
  
  l2_offset = 0x0;

  // initialize the arrays
  for(i=0; i<ARM_DMA_SIZE; i++) {
    arm_dma_array[i] = i+1;
  }
  for(i=0; i<PULP_DMA_SIZE; i++) {
    pulp_dma_array[i] = i+1;
  }

  /*
   * First, test the Zynq PS DMA engine.
   */
  printf("Testing Zynq PS DMA engine...\n");
  printf("Source: %d Bytes starting at %p\n",ARM_DMA_SIZE_B,arm_dma_array);
  sleep(1);
 
  // clean L2
  for (i=0; i<ARM_DMA_SIZE; i++) {
#ifndef USE_L3
    pulp_write32(pulp->l2_mem.v_addr,l2_offset+4*i,'b',0);
#else
    pulp_write32(pulp->l3_mem.v_addr,l2_offset+4*i,'b',0);
#endif
  }
 
  clock_gettime(CLOCK_REALTIME,&tp1);

#ifndef USE_L3
  pulp_dma_xfer(pulp,(unsigned)&arm_dma_array,0x4C000000+l2_offset,ARM_DMA_SIZE_B,0);
#else
  pulp_dma_xfer(pulp,(unsigned)&arm_dma_array,0x38000000+l2_offset,ARM_DMA_SIZE_B,0);
#endif  

  clock_gettime(CLOCK_REALTIME,&tp2);

  // compute bandwidth
  s_duration = (int)(tp2.tv_sec - tp1.tv_sec);
  if ((tp2.tv_nsec > tp1.tv_nsec) && (s_duration < 1)) { // no overflow
    ns_duration = (tp2.tv_nsec - tp1.tv_nsec);
  }
  else {//((tp2.tv_nsec < tp1.tv_nsec) && (s_duration > 1)) {// overflow of tv_nsec
    ns_duration = (1000000000 - tp1.tv_nsec + tp2.tv_nsec) % 1000000000;
    s_duration = (1000000000 - tp1.tv_nsec + tp2.tv_nsec) / 1000000000;
  }
  printf("Measured Zynq PS DMA bandwidth = %.2f MB/s (includes transfer setup)\n",((float)(ARM_DMA_SIZE_B)/((float)(ns_duration+s_duration*1000000000)))*1000000000/(1024*1024));

#ifdef CHECK_RESULT
  int value;

  for (i=0; i<ARM_DMA_SIZE; i++) {
#ifndef USE_L3
    value = pulp_read32(pulp->l2_mem.v_addr,l2_offset+4*i,'b');
#else
    value = pulp_read32(pulp->l3_mem.v_addr,l2_offset+4*i,'b');
#endif   
    //printf("entry %d = %d \n",i,value);
    if ( arm_dma_array[i] != value ) {
      printf("Error: mismatch in Entry %i detected\n",i);
      printf("Value found in L2 = %d\n",value);
      printf("Value expected    = %d\n",arm_dma_array[i]);
      break;
    }
    if ( i == ARM_DMA_SIZE-1 )
      printf("Success: L2 content matches array in DRAM.\n");
  }
#endif // CHECK_RESULT

  /*
   * Second, run a test program on PULP to test it's own DMA engine.
   */
  printf("Testing PULP DMA engine...\n");  
  
  // manually setup data structures (no compiler support right now)
  TaskDesc task_desc;
   
  char name[30];
  strcpy(name,"dma_performance_measurement");
  
  task_desc.name = &name[0];
  task_desc.n_clusters = -1;
  task_desc.n_data = 1;
   
  DataDesc data_desc[task_desc.n_data];
  data_desc[0].ptr = (unsigned *)&pulp_dma_array;
  data_desc[0].size   = PULP_DMA_SIZE_B;
  
  task_desc.data_desc = data_desc;
  
  sleep(1);
  
  clock_gettime(CLOCK_REALTIME,&tp1);

  // issue the offload
  pulp_omp_offload_task(pulp,&task_desc);
  //pulp_omp_wait(pulp,ker_id);
  
  clock_gettime(CLOCK_REALTIME,&tp2);

  // compute bandwidth
  s_duration = (int)(tp2.tv_sec - tp1.tv_sec);
  if ((tp2.tv_nsec > tp1.tv_nsec) && (s_duration < 1)) { // no overflow
    ns_duration = (tp2.tv_nsec - tp1.tv_nsec);
  }
  else {//((tp2.tv_nsec < tp1.tv_nsec) && (s_duration > 1)) {// overflow of tv_nsec
    ns_duration = (1000000000 - tp1.tv_nsec + tp2.tv_nsec) % 1000000000;
    s_duration = (1000000000 - tp1.tv_nsec + tp2.tv_nsec) / 1000000000;
  }
  printf("Measured PULP DMA bandwidth = %.2f MB/s (includes offload and transfer setup)\n",((float)(ARM_DMA_SIZE_B)/((float)(ns_duration+s_duration*1000000000)))*1000000000/(1024*1024));

  int time_high, time_low;
  time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');

  float time;
  time = ((2<<31)*(float)time_high + (float)time_low)/(50);

  printf("PULP - DMA: size = %d [bytes]\n",PULP_DMA_SIZE_B);
  printf("PULP - DMA: time = %.2f [us]\n",time);

  float bw;
  bw = PULP_DMA_SIZE_B/time;
  printf("DMA bandwidth measured on PULP = %.2f MiB/s\n",bw);

#ifdef CHECK_RESULT
  unsigned address, temp;
  address = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
 
  address -= L2_MEM_BASE_ADDR;

  for (i=0; i<PULP_DMA_SIZE; i++) {
    temp = address + i*4;
    value = pulp_read32(pulp->l2_mem.v_addr,temp,'b');
    //printf("entry %d = %d \n",i,value);
    if ( pulp_dma_array[i] != value ) {
      printf("Error: mismatch in Entry %i detected\n",i);
      printf("Value found in L2 = %d\n",value);
      printf("Value expected    = %d\n",arm_dma_array[i]);
      break;
    }
    if ( i == PULP_DMA_SIZE-1 )
      printf("Success: L2 content matches array in DRAM.\n");
  }
#endif // CHECK_RESULT
 
  //free(arm_dma_array);
  //free(pulp_dma_array);

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
