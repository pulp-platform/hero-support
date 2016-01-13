#define N_ELEMENTS 3

#include <unistd.h> // for sleep
#include <stdlib.h> // posix_memalign
#include <time.h>   // for time measurements

#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"

#include "zynq_pmm_user.h"

//#define ARRAY_SIZE_B (RAB_N_SLICES-2)/N_ELEMENTS/2 *0x1000
#define ARRAY_SIZE_B 0x1000 * 1000 
#define ARRAY_SIZE   ARRAY_SIZE_B/4

int main(int argc, char *argv[]) {
  
  int i,j,ret;
   
  printf("Profiling RAB...\n");

  /*
   * Preparation
   */
  char app_name[30];
  int timeout_s = 20;  
  int pulp_clk_freq_mhz = 50;

  strcpy(app_name,"profile_rab");
  
  if (argc > 3) {
    printf("WARNING: More than 2 command line argument is not supported. Those will be ignored.\n");
  }

  if (argc > 1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  if (argc > 2)
    timeout_s = atoi(argv[2]);

  // shared data element
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
  if (ret > 0) {
    printf("PULP Running @ %d MHz.\n",ret);
    pulp_clk_freq_mhz = ret;
  }
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
  
  // manually setup data structures (no compiler support right now)
  TaskDesc task_desc;
   
  task_desc.name = &app_name[0];
  task_desc.n_clusters = -1;
  task_desc.n_data = N_ELEMENTS;
   
  DataDesc data_desc[N_ELEMENTS];
  for (i=0; i<task_desc.n_data; i++) {
    data_desc[i].ptr  = array_ptrs[i];
    data_desc[i].size = ARRAY_SIZE_B;  
  }
    
  task_desc.data_desc = data_desc;
  
  sleep(1);
  
  /*
   * Body
   */
  printf("PULP Execution\n");

  // issue the offload
  pulp_omp_offload_task(pulp,&task_desc);
  //pulp_omp_wait(pulp,ker_id);
  
  pulp_rab_free_striped(pulp);

  sleep(1);

  unsigned clk_cntr_response, clk_cntr_update, clk_cntr_setup, clk_cntr_cleanup;
  unsigned clk_cntr_cache_flush, clk_cntr_get_user_pages, clk_cntr_map_sg;
  unsigned n_slices_updated, n_pages_setup, n_updates, n_cleanups;

  // read variables from shared memory
  clk_cntr_response       = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_RESPONSE_OFFSET_B,'b');
  clk_cntr_update         = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_UPDATE_OFFSET_B,'b');
  clk_cntr_setup          = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_SETUP_OFFSET_B,'b');
  clk_cntr_cleanup        = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_CLEANUP_OFFSET_B,'b');
  clk_cntr_cache_flush    = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_CACHE_FLUSH_OFFSET_B,'b');
  clk_cntr_get_user_pages = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_GET_USER_PAGES_OFFSET_B,'b');
  clk_cntr_map_sg         = pulp_read32(pulp->l3_mem.v_addr,CLK_CNTR_MAP_SG_OFFSET_B,'b');
  n_updates               = pulp_read32(pulp->l3_mem.v_addr,N_UPDATES_OFFSET_B,'b');
  n_slices_updated        = pulp_read32(pulp->l3_mem.v_addr,N_SLICES_UPDATED_OFFSET_B,'b');
  n_pages_setup           = pulp_read32(pulp->l3_mem.v_addr,N_PAGES_SETUP_OFFSET_B,'b');
  n_cleanups              = pulp_read32(pulp->l3_mem.v_addr,N_CLEANUPS_OFFSET_B,'b');

  float response_time, update_time, setup_time, cleanup_time;
  response_time = ((float)clk_cntr_response)/(n_updates*pulp_clk_freq_mhz);
  update_time = ((float)clk_cntr_update*64)/(n_slices_updated*ARM_CLK_FREQ_MHZ);
  setup_time = ((float)clk_cntr_setup*64)/(n_pages_setup*ARM_CLK_FREQ_MHZ);
  cleanup_time = ((float)clk_cntr_cleanup*64)/(n_pages_setup*ARM_CLK_FREQ_MHZ);
  
  float cache_flush_time, get_user_pages_time, map_sg_time;
  cache_flush_time = ((float)clk_cntr_cache_flush*64)/(n_pages_setup*ARM_CLK_FREQ_MHZ);
  get_user_pages_time = ((float)clk_cntr_get_user_pages*64)/(n_pages_setup*ARM_CLK_FREQ_MHZ);
  map_sg_time = ((float)clk_cntr_map_sg*64)/(n_pages_setup*ARM_CLK_FREQ_MHZ);
  
  printf("----------------------------------------------------------\n");
  printf("Response time (per update request) = %.3f us\n",response_time);
  printf("Update time (per slice)            = %.3f us\n",update_time);
  printf("Setup time (per page)              = %.3f us\n",setup_time);
  printf("Cleanup time (per page)            = %.3f us\n",cleanup_time);
  printf("----------------------------------------------------------\n");
  printf("Cache flush time (per page)        = %.3f us\n",cache_flush_time);
  printf("Get user pages time (per page)     = %.3f us\n",get_user_pages_time);
  printf("Map segment time (per page)        = %.3f us\n",map_sg_time);
  printf("----------------------------------------------------------\n");

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

  usleep(10000);

  /*
   * Cleanup
   */
  /*************************************************************************/
  for (i=0; i<N_ELEMENTS; i++) {
    free(array_ptrs[i]);
  }
  /*************************************************************************/

  printf("PULP Cleanup\n");
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
  
  return 0;
}
