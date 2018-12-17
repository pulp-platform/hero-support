/*
 * bigPULP standalone application launcher
 *
 * Copyright 2018 ETH Zurich, University of Bologna
 *
 * Author: Pirmin Vogel <vogelpi@iis.ee.ethz.ch>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>

#include "pulp.h"

int main(int argc, char *argv[]) {

  int ret;
  
  printf("bigPULP Standalone Application Launcher\n");
 
  /*
   * Preparation
   */
  char app_name[50];
  unsigned int timeout_s = 1;
  unsigned int pulp_cluster_sel = 0x1;
  unsigned int pulp_clk_freq_mhz = PULP_DEFAULT_FREQ_MHZ;
  
  if (argc < 2) {
    printf("ERROR: Specify the name of the standalone PULP application to execute as first argument Type -h for help.\n");
    return -EINVAL;
  }
  else if ( (argc == 2) && !strcmp("-h",argv[1]) ) {
    printf("Pirmin Vogel - vogelpi@iis.ee.ethz.ch\n\n");
    printf("Argument Nr. - Meaning             - Example   - Default\n");
    printf("--------------------------------------------------------\n");
    printf("1 - application name               - test/test - - \n");
    printf("2 - timeout in seconds             - 10        - 1 \n");
    printf("3 - cluster select bit mask        - 15        - 1 \n");
    printf("4 - target clock frequency in MHz  - 20        - DEFAULT \n");
    return -EINVAL;
  }
  else if (argc > 5) {
    printf("WARNING: More than 4 command line arguments are not supported. Those will be ignored. Type -h for help.\n");
  }

  strcpy(app_name,argv[1]);
  if (argc > 2)
    timeout_s = atoi(argv[2]);
  if (argc > 3)
    pulp_cluster_sel = atoi(argv[3]);    
  if (argc > 4)
    pulp_clk_freq_mhz = atoi(argv[4]);

  /*
   * Initialization
   */
  printf("PULP Initialization\n");
 
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  pulp->cluster_sel = pulp_cluster_sel;

  // reserve virtual addresses overlapping with PULP's internal physical address space
  pulp_reserve_v_addr(pulp);

  // memory map the device
  pulp_mmap(pulp);
  
  //pulp_print_v_addr(pulp);
  
  pulp_reset(pulp,1);
  
  // set desired clock frequency
  ret = pulp_clking_set_freq(pulp,pulp_clk_freq_mhz);
  if (ret > 0)
    printf("PULP configured to run @ %d MHz.\n",ret);
  else
    printf("ERROR: setting clock frequency failed");
  
  pulp_rab_free(pulp,0x0);

  // initialization of PULP, static RAB rules (mailbox, L2, ...)
  pulp_init(pulp);

  // measure the actual clock frequency
  printf("PULP actually running @ %d MHz.\n",pulp_clking_measure_freq(pulp));

  // enable forwarding TLB invalidations
  printf("PULP enable invalidation forwarding\n");
  pulp_rab_inv_enable(pulp);

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
 
  /*
   * Cleanup
   */
  printf("PULP Cleanup\n");
  pulp_rab_inv_disable(pulp);
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
