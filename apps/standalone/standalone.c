#include <stdlib.h>

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

int main(int argc, char *argv[]) {

  int ret;
  
  printf("PULP Standalone Application Launcher\n");
 
  /*
   * Preparation
   */
  char app_name[50];
  int timeout_s = 1;  
  int pulp_clk_freq_mhz = 50;
  
  if (argc < 2) {
    printf("ERROR: Specify the name of the standalone PULP application to execute as first argument.\n");
    return -EINVAL;
  }
  else if (argc > 4) {
    printf("WARNING: More than 3 command line arguments are not supported. Those will be ignored.\n");
  }

  strcpy(app_name,argv[1]);
  if (argc > 2)
    timeout_s = atoi(argv[2]);    
  if (argc > 3)
    pulp_clk_freq_mhz = atoi(argv[3]);    

  /*
   * Initialization
   */
  printf("PULP Initialization\n");
 
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  // reserve virtual addresses overlapping with PULP's internal physical address space
  pulp_reserve_v_addr(pulp);

  pulp_mmap(pulp);
  //pulp_print_v_addr(pulp);
  pulp_reset(pulp,1);
  
  // set desired clock frequency
  ret = pulp_clking_set_freq(pulp,pulp_clk_freq_mhz);
  if (ret > 0)
    printf("PULP confgiured to run @ %d MHz.\n",ret);
  else
    printf("ERROR: setting clock frequency failed");
  
  pulp_rab_free(pulp,0x0);

  // initialization of PULP, static RAB rules (mailbox, L2, ...)
  pulp_init(pulp);

  // measure the actual clock frequency
  printf("PULP actually running @ %d MHz.\n",pulp_clking_measure_freq(pulp));

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