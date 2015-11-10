#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

//#define USE_L3
#define CHECK_RESULT

#ifndef USE_L3
#define ARM_DMA_SIZE_B L2_MEM_SIZE_B
#else
#define ARM_DMA_SIZE_B 1572864
#endif
//#define ARM_DMA_SIZE_B 0x8000
//#define ARM_DMA_SIZE_B 0x1000
#define PULP_DMA_SIZE_B 0x4000 //L1_MEM_SIZE_B/2

#define ARM_DMA_SIZE  ARM_DMA_SIZE_B/4
#define PULP_DMA_SIZE  PULP_DMA_SIZE_B/4

int main(int argc, char *argv[]){
  
  int ret;
  
  printf("Testing DMA performance...\n");

  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // reserve virtual addresses overlapping with PULP's internal physical address space
  pulp_reserve_v_addr(pulp);
  
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

  int pulp_clk_freq_mhz = 50;

  if (argc > 2) {
    printf("WARNING: More than 1 command line argument is not supported. Those will be ignored.\n");
  }
  
  if (argc > 1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  /*
   * Initialization
   */ 
  printf("PULP Initialization\n");
  
  pulp_mmap(pulp);
  //sleep(1);
  //pulp_print_v_addr(pulp);
  //sleep(1);  
  pulp_reset(pulp,1);

  // set desired clock frequency
  if (pulp_clk_freq_mhz != 50) {
    ret = pulp_clking_set_freq(pulp,pulp_clk_freq_mhz);
    if (ret > 0)
      printf("PULP confgiured to run @ %d MHz.\n",ret);
    else
      printf("ERROR: setting clock frequency failed");
  }

  pulp_rab_free(pulp,0x0);

  // initialization of PULP, static RAB rules (mailbox, L2, ...)
  pulp_init(pulp);

  // measure the actual clock frequency
  pulp_clk_freq_mhz = pulp_clking_measure_freq(pulp);
  printf("PULP actually running @ %d MHz.\n",pulp_clk_freq_mhz);

  /*
   * Body
   */
  int i;
  unsigned l2_offset;
  
  l2_offset = 0x0;

#ifdef CHECK_RESULT
  int value;
#endif

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
  printf("Measured Zynq PS DMA bandwidth = %.2f MiB/s (includes transfer setup)\n",
	 ((float)(ARM_DMA_SIZE_B)/((float)(ns_duration+s_duration*1000000000)))*1000000000/(1024*1024));

#ifdef CHECK_RESULT
  for (i=0; i<ARM_DMA_SIZE; i++) {
#ifndef USE_L3
    value = pulp_read32(pulp->l2_mem.v_addr,l2_offset+4*i,'b');
#else
    value = pulp_read32(pulp->l3_mem.v_addr,l2_offset+4*i,'b');
#endif   
    //printf("entry %d = %d \n",i,value);
    if ( arm_dma_array[i] != value ) {
      printf("Error: mismatch in Entry %i detected\n",i);
      printf("Value found    = %d\n",value);
      printf("Value expected = %d\n",arm_dma_array[i]);
      break;
    }
    if ( i == ARM_DMA_SIZE-1 )
      printf("Success: Content matches.\n");
  }
#endif // CHECK_RESULT
  /*
   * Second, let the CPU do a memcpy().
   */
  printf("Testing Zynq PS memcpy()...\n");  

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
  memcpy((void *)(pulp->l2_mem.v_addr+l2_offset),(void *)arm_dma_array,ARM_DMA_SIZE_B);
#else
  memcpy((void *)(pulp->l3_mem.v_addr+l2_offset),(void *)arm_dma_array,ARM_DMA_SIZE_B);
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
  printf("Measured Zynq PS memcpy() bandwidth = %.2f MiB/s\n",
	 ((float)(ARM_DMA_SIZE_B)/((float)(ns_duration+s_duration*1000000000)))*1000000000/(1024*1024));

#ifdef CHECK_RESULT
  for (i=0; i<ARM_DMA_SIZE; i++) {
#ifndef USE_L3
    value = pulp_read32(pulp->l2_mem.v_addr,l2_offset+4*i,'b');
#else
    value = pulp_read32(pulp->l3_mem.v_addr,l2_offset+4*i,'b');
#endif   
    //printf("entry %d = %d \n",i,value);
    if ( arm_dma_array[i] != value ) {
      printf("Error: mismatch in Entry %i detected\n",i);
      printf("Value found     = %d\n",value);
      printf("Value expected  = %d\n",arm_dma_array[i]);
      break;
    }
    if ( i == ARM_DMA_SIZE-1 )
      printf("Success: Content matches.\n");
  }
#endif // CHECK_RESULT

  /*
   * Third, run a test program on PULP to test it's own DMA engine.
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
  printf("Measured PULP DMA bandwidth = %.2f MiB/s (includes offload and transfer setup)\n",
	 ((float)(ARM_DMA_SIZE_B)/((float)(ns_duration+s_duration*1000000000)))*1000000000/(1024*1024));

  int time_high, time_low;
  //time_high = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  pulp_mailbox_read(pulp,(unsigned *)&time_high,1);
  //time_low = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  pulp_mailbox_read(pulp,(unsigned *)&time_low,1);

  float time;
  time = ((2<<31)*(float)time_high + (float)time_low)/(pulp_clk_freq_mhz);

  printf("PULP - DMA: size = %d [bytes]\n",PULP_DMA_SIZE_B);
  printf("PULP - DMA: time = %.2f [us]\n",time);

  float bw;
  bw = (float)PULP_DMA_SIZE_B/time*1000000/(1024*1024);
  printf("DMA bandwidth measured on PULP = %.2f MiB/s\n",bw);

#ifdef CHECK_RESULT
  unsigned address, temp;
  //address = pulp_read32(pulp->mailbox.v_addr,MAILBOX_RDDATA_OFFSET_B,'b');
  pulp_mailbox_read(pulp,&address,1);

  address -= L2_MEM_BASE_ADDR;

  for (i=0; i<PULP_DMA_SIZE; i++) {
    temp = address + i*4;
    value = pulp_read32(pulp->l2_mem.v_addr,temp,'b');
    //printf("entry %d = %d \n",i,value);
    if ( pulp_dma_array[i] != value ) {
      printf("Error: mismatch in Entry %i detected\n",i);
      printf("Value found    = %d\n",value);
      printf("Value expected = %d\n",arm_dma_array[i]);
      break;
    }
    if ( i == PULP_DMA_SIZE-1 )
      printf("Success: Content matches.\n");
  }
#endif // CHECK_RESULT
 
  // wait for end of computation
  pulp_exe_wait(pulp,2);

  // stop execution
  pulp_exe_stop(pulp);

  // -> poll stdout
  pulp_stdout_print(pulp,0);
  pulp_stdout_print(pulp,1);
  pulp_stdout_print(pulp,2);
  pulp_stdout_print(pulp,3);

  // clear stdout
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  /*
   * Cleanup
   */
  printf("PULP Cleanup\n");
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
