#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

int main(int argc, char *argv[]){
  
  int ret;
  
  printf("Testing PULP memory latencies...\n");      

  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // reserve virtual addresses overlapping with PULP's internal physical address space
  pulp_reserve_v_addr(pulp);

  /*
   * Preparation
   */
  char app_name[30];
  int timeout_s = 10;  
  int pulp_clk_freq_mhz = 50;

  strcpy(app_name,"latency_measurement");
  
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
  //pulp_print_v_addr(pulp);
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

  // clear memories?
  
  // clear stdout
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  /*
   * Body
   */
  printf("PULP Execution\n");
  // load binary
  pulp_load_bin(pulp,app_name);
 
  // start execution
  pulp_exe_start(pulp);

  /*************************************************************************/

  unsigned int time_high, time_low;
  float time, cycles;
 
  pulp_mailbox_read(pulp,&time_high,1);
  pulp_mailbox_read(pulp,&time_low,1);
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/pulp_clk_freq_mhz;
  printf("PULP - L1: read latency = %.2f [cycles]\n",cycles);
  printf("PULP - L1: read latency = %.2f [us]\n",time);

  pulp_mailbox_read(pulp,&time_high,1);
  pulp_mailbox_read(pulp,&time_low,1);
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/pulp_clk_freq_mhz;
  printf("PULP - L1: write latency = %.2f [cycles]\n",cycles);
  printf("PULP - L1: write latency = %.2f [us]\n",time);

  pulp_mailbox_read(pulp,&time_high,1);
  pulp_mailbox_read(pulp,&time_low,1);
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/pulp_clk_freq_mhz;
  printf("PULP - L2: read latency = %.2f [cycles]\n",cycles);
  printf("PULP - L2: read latency = %.2f [us]\n",time);

  pulp_mailbox_read(pulp,&time_high,1);
  pulp_mailbox_read(pulp,&time_low,1);
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/pulp_clk_freq_mhz;
  printf("PULP - L2: write latency = %.2f [cycles]\n",cycles);
  printf("PULP - L2: write latency = %.2f [us]\n",time);

  pulp_mailbox_read(pulp,&time_high,1);
  pulp_mailbox_read(pulp,&time_low,1);
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/pulp_clk_freq_mhz;
  printf("PULP - L3: read latency = %.2f [cycles]\n",cycles);
  printf("PULP - L3: read latency = %.2f [us]\n",time);

  pulp_mailbox_read(pulp,&time_high,1);
  pulp_mailbox_read(pulp,&time_low,1);
  cycles = ((2<<31)*(float)time_high + (float)time_low);
  time = cycles/pulp_clk_freq_mhz;
  printf("PULP - L3: write latency = %.2f [cycles]\n",cycles);
  printf("PULP - L3: write latency = %.2f [us]\n",time);

  /*************************************************************************/

  // wait for end of computation
  pulp_exe_wait(pulp,timeout_s);

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
