#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

#define PAGE_SIZE 4096

#define N_ELEMENTS_PER_CORE 2

#define ARRAY_SIZE_B PAGE_SIZE*N_CORES*N_ELEMENTS_PER_CORE

int main(int argc, char *argv[]){
  
  int i,j,ret;
  unsigned status;
  
  printf("Testing RAB miss handling...\n");      

  /*
   * Preparation
   */
  char app_name[30];
  int timeout_s = 10;  
  int pulp_clk_freq_mhz = 50;

  strcpy(app_name,"rab_miss_handling");
  
  if (argc > 2) {
    printf("WARNING: More than 1 command line argument is not supported. Those will be ignored.\n");
  }

  if (argc > 1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  // shared data element
  unsigned *array_ptr;
  posix_memalign((void **)&array_ptr,(size_t)8,(size_t)ARRAY_SIZE_B);
  if (array_ptr == NULL) {
    printf("ERROR: posix_memalign failed.\n");
    return 1;
  }
  for (i=0; i<ARRAY_SIZE_B/4; i++)
    *(array_ptr + i) = i;

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
  if (pulp_clk_freq_mhz != 50) {
    ret = pulp_clking_set_freq(pulp,pulp_clk_freq_mhz);
    if (ret > 0) {
      printf("PULP Running @ %d MHz.\n",ret);
      pulp_clk_freq_mhz = ret;
    }
    else
      printf("ERROR: setting clock frequency failed");
  }

  pulp_rab_free(pulp,0x0);

  // initialization of PULP, static RAB rules (mailbox, L2, ...)
  pulp_init(pulp);

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

  /*************************************************************************/
 
  // enable RAB miss handling
  pulp_rab_mh_enable(pulp);
  
  // write PULP_START
  pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', PULP_START);

  // pass pointers to mailbox
  for (i=0; i<N_CORES; i++) {
    for (j=0; j<N_ELEMENTS_PER_CORE; j++) {
      status = 1;
      while (status) 
	status = pulp_read32(pulp->mailbox.v_addr, MAILBOX_STATUS_OFFSET_B, 'b') & 0x2;	
      pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b',
		   (unsigned)array_ptr+PAGE_SIZE*(i*N_ELEMENTS_PER_CORE+j));
      printf("Wrote address %#x of value %#x to mailbox.\n",
	     (unsigned)array_ptr+PAGE_SIZE*(i*N_ELEMENTS_PER_CORE+j),
	     *(array_ptr+PAGE_SIZE/4*(i*N_ELEMENTS_PER_CORE+j)));
    }
  }
   
  /*************************************************************************/

  // start execution
  pulp_exe_start(pulp);
  
  usleep(2000000);

  // wait for end of computation
  pulp_exe_wait(pulp,timeout_s);

  // stop execution
  pulp_exe_stop(pulp);

  /*************************************************************************/

  // disable RAB miss handling
  pulp_rab_mh_disable(pulp);

  /*************************************************************************/

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
  /*************************************************************************/
  free(array_ptr);
  /*************************************************************************/  

  printf("PULP Cleanup\n");
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
