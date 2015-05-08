#define N_ELEMENTS 4

#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

#include "zynq_pmm_user.h"

//#define ARRAY_SIZE_B (RAB_N_SLICES-2)/N_ELEMENTS/2 *0x1000
//#define ARRAY_SIZE_B 0x1000 * 1000 
#define ARRAY_SIZE_B 0x1000 * 8 
#define ARRAY_SIZE   ARRAY_SIZE_B/4

#define PULP_CLK_FREQ_MHZ 75
//#define PULP_CLK_FREQ_MHZ 50

int main(){
  
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
  printf("PULP running at %d MHz\n",pulp_clking_set_freq(pulp,PULP_CLK_FREQ_MHZ));
  pulp_rab_free(pulp,0x0);
  pulp_init(pulp);
  
  /*
   * Body
   */
  int i,j;

  unsigned * array_ptrs[N_ELEMENTS];

  // allocate memory and initialize arrays
  for (i=0; i<N_ELEMENTS; i++) {
    posix_memalign((void **)&array_ptrs[i],(size_t)8,(size_t)ARRAY_SIZE_B);
    if (array_ptrs[i] == NULL) {
      printf("ERROR: posix_memalign failed.\n");
    }
    for(j=0; j<ARRAY_SIZE; j++) {
      *(array_ptrs[i] + j) = j+1;
    }
  }
  
  /*
   * Run a test program on PULP
   */
  printf("Testing DMA...\n");  
  
  // manually setup data structures (no compiler support right now)
  TaskDesc task_desc;
   
  char name[15];
  strcpy(name,"dma_test");
  
  task_desc.name = &name[0];
  task_desc.n_clusters = -1;
  task_desc.n_data = N_ELEMENTS;
   
  DataDesc data_desc[task_desc.n_data];
  for (i=0; i<task_desc.n_data; i++) {
    data_desc[i].ptr  = array_ptrs[i];
    data_desc[i].size = ARRAY_SIZE_B;  
  }
    
  task_desc.data_desc = data_desc;
  
  sleep(1);
  
  // issue the offload
  pulp_omp_offload_task(pulp,&task_desc);
  //pulp_omp_wait(pulp,ker_id);
  
  pulp_rab_free_striped(pulp);

  pulp_stdout_print(pulp,0);
  pulp_stdout_print(pulp,1);
  pulp_stdout_print(pulp,2);
  pulp_stdout_print(pulp,3);
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  sleep(3);

  /*
   * Cleanup
   */

  for (i=0; i<N_ELEMENTS; i++) {
    free(array_ptrs[i]);
  }

  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
