#include <unistd.h> // for sleep
#include <stdlib.h> // for malloc
#include <math.h>

#include "zynq.h"
#include "zynq_pmm_user.h"

int main(int argc, char *argv[]){

  double fixed_miss_rate = 1;
  unsigned zynq_pmm = 0;
  unsigned loop_forever = 1;
  //#define WRITE_L3

  if (argc < 3) {
    if (argc == 2)
      fixed_miss_rate = strtod(argv[1],NULL);
  }
  else  {
    printf("Please specify exactly one input argument, i.e., the desired cache miss rate.\n");
    return -1;
  }  
 
  // Loop variables
  unsigned i,j,k;
  unsigned n_chunks,n_repetitions,n_tests;
  n_repetitions = 100;
  n_chunks = 1;
  
  // Setup cache miss rate measurement
  int *fd_ptr;
  int fd,ret;
  char proc_text[ZYNQ_PMM_PROC_N_CHARS_MAX];
  long long counter_values[(N_ARM_CORES+1)*2];
  double cache_miss_rates[(N_ARM_CORES+1)*2];
 
  // open zynq_pmm
  if (zynq_pmm) {
    fd_ptr = &fd;
    ret = zynq_pmm_open(fd_ptr);
    if (ret)
      return ret;
  }
 
  // Setup time measurement
  volatile unsigned clk_cntr_start, clk_cntr_end, clk_cntr_acc;
  unsigned overhead, overhead_acc;
  overhead = arm_clk_cntr_get_overhead();
  printf("Counter overhead = %u CPU clocks\n",overhead);
 
  // Compute Bandwidth
  double n_clks,us_duration,cpu_bw;

  // Array Size
  //unsigned test_size_b = 4*1024*1024; // eight times L2, to allow for 100% miss rate
  //unsigned test_size_b = 128*1024*1024;
  //unsigned test_size_b = 1024*1024;
  //unsigned test_size_b = 256*1024; // eight times L1, to allow for 100% miss rate
  unsigned test_size_b = 16*1024*1024;
  
  // Number of tests
  // Shift address back by 0 cache lines (100% miss rate) and 64 lines (0% miss rate)
  unsigned max_shift = 64;
  unsigned step = 1;
  n_tests = max_shift/step + 1;
  unsigned *n_lines_shift = (unsigned *) malloc((n_tests)*sizeof(unsigned));
  double *miss_rate = (double *) malloc((n_tests)*sizeof(double));
  double *bw_vec = (double *) malloc((n_tests)*sizeof(double));
  double *eff_global_mr_vec = (double *) malloc((n_tests)*sizeof(double));
  for (i=0;i<n_tests-1;i++) {
    n_lines_shift[i] = max_shift-(i*step);
    miss_rate[i] = ((double) i*step)/64;
  }
  n_lines_shift[n_tests-1] = 0;
  miss_rate[n_tests-1] = 1;

  // Setup the test
  unsigned shift;
  unsigned chunk_size_b;
  unsigned word_size_b;
  unsigned test_size_w;
  unsigned *dram_ptr;
  
  // Variables for inline assembly
  unsigned address;
  const unsigned value = 0xF;
  unsigned variable = 0xF;
  unsigned variable2 = 0xA;
  
  // loop over different memory sizes
  //for (k=0; k<n_tests; k++) {

  printf("Desired Miss Rate: %.2f\n",fixed_miss_rate);

  k = 0;
  for (i=0; i<n_tests; i++) {
    if (fixed_miss_rate >= miss_rate[i]) {
      k = i;
    }
    else {
      break;
    }
  }
    
  printf("Effective Miss Rate: %.2f\n",miss_rate[k]);

  // Extract the number of lines to shift
  shift = n_lines_shift[k];
    
  // Determine the chunk size, we do 64 cache lines per chunk
  // Both L1 and L2 use 32-byte cache lines
  chunk_size_b = 64*32;
  if (shift == 0 || shift == 64) {
    n_chunks = test_size_b/chunk_size_b;
  }
  else {
    //n_chunks = 1+(test_size_b - chunk_size_b)/(chunk_size_b - shift*32);
    n_chunks = (test_size_b)/(chunk_size_b - shift*32);
  }
  //n_chunks = test_size_b/chunk_size_b;

  // Allocate some memory in the cached DRAM
  word_size_b = sizeof(unsigned);
  test_size_w = test_size_b/word_size_b;
  dram_ptr = (unsigned *)malloc(test_size_w*word_size_b);

  // Write the allocated memory
  for (i=0;i<test_size_w;i++) {
    *(dram_ptr+i) = 4*i;
  }
  
  // Prepare time measurement
  clk_cntr_acc = 0;
  clk_cntr_start = 0;
  clk_cntr_end = 0;

  if (zynq_pmm) {
    // Prepare cache measurement
    // delete the accumulation variables
    ret = zynq_pmm_parse(proc_text, counter_values, -1);
    // reset the hardware counters
    ret = zynq_pmm_read(fd_ptr,proc_text);  
    if (ret)
      return ret;
  }        

  // Info
  printf("Shift = %u cache lines, Number of Chunks = %u, Miss Rate = %.2f\n",n_lines_shift[k],n_chunks,miss_rate[k]);

  // repeat the same test several times
  for (j=0; j<n_repetitions; j++) {
    if (loop_forever)
      j=0;
   
    // Prepare the address pointer
    address = (unsigned)dram_ptr;
 
    // Info
    //printf("Repetition %u, address = %#x \n",j,address);
      
    arm_clk_cntr_reset();

    // repeat n_chunk times
    for (i = 0; i < n_chunks; i++) {

      // Info
      //printf("Chunk %u, address = %#x \n",i,address);

      // time measurement
      clk_cntr_start = arm_clk_cntr_read();

#ifdef WRITE_L3

#include "str_64_lines.c"

#else // READ_L3

#include "ldr_64_lines.c"

#endif

	// time measurement
	clk_cntr_end = arm_clk_cntr_read();
	// accumulate time
	clk_cntr_acc += (clk_cntr_end - clk_cntr_start);
	// shift the address
	address -= (shift * 32);
    }

    if (zynq_pmm) {
      // cache measurement
      ret = zynq_pmm_read(fd_ptr, proc_text);
      if (ret)
	return ret;
      ret = zynq_pmm_parse(proc_text, counter_values, 1);
    }

    // Info
    //printf("Repetition %u, address = %#x \n",j,address);
  }   

  // free memory
  free(dram_ptr);
    
  // Compute bandwidth
  //overhead_acc = n_chunks*n_repetitions*overhead;
  //n_clks = (double)((clk_cntr_acc-overhead_acc)*ARM_PMU_CLK_DIV);
  n_clks = (double)clk_cntr_acc*ARM_PMU_CLK_DIV;
  //printf("n_clks %.2f \n",n_clks);
  us_duration = n_clks/ARM_CLK_FREQ_MHZ;
  //us_duration = n_clks/667;
  //printf("us_duration %.2f \n",us_duration);
  cpu_bw = (double)(word_size_b*64*(double)n_chunks*(double)n_repetitions)/us_duration/1000;
  printf("Measured CPU Bandwidth = %.2f GiB/s\n",cpu_bw);
  bw_vec[k] = cpu_bw; 

  if (zynq_pmm) {
    // Compute miss rates
    ret = zynq_pmm_compute_rates(cache_miss_rates, counter_values);
    ret = zynq_pmm_print_rates(cache_miss_rates);
    eff_global_mr_vec[k] = cache_miss_rates[(N_ARM_CORES+1)*2-1];
    printf("Measured Cache Miss Rate (Read) = %.2f \n",eff_global_mr_vec[k]);
  }
  //}

  // free memory
  free(bw_vec);
  free(miss_rate);
  free(n_lines_shift);
      
  if (zynq_pmm) { 
    sleep(1);
    zynq_pmm_close(fd_ptr);
  }
  
  return 0;
}
