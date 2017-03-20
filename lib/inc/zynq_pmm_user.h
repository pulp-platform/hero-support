#ifndef ZYNQ_PMM_USER__
#define ZYNQ_PMM_USER__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h> // for error codes
#include <unistd.h>
#include <math.h>

#include "zynq.h"

// function prototypes
int zynq_pmm_open(int *fd);
void zynq_pmm_close(int *fd);

int zynq_pmm_read(int *fd, char *buffer);
int zynq_pmm_parse(char *buffer, long long *counters, int accumulate);
int zynq_pmm_compute_rates(double *miss_rates, long long *counters);
int zynq_pmm_print_rates(double *miss_rates);

unsigned arm_clk_cntr_get_overhead();

/*
 * Reset the clock cycle counter in the PMU
 */
inline void arm_clk_cntr_reset()
{
  // enable clock counter divider (by 64), reset & enable clock counter, PMCR register
  asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0xD));
}

/*
 * Read the clock cycle counter in the PMU
 */
inline unsigned arm_clk_cntr_read()
{
  unsigned value;
  // Read the counter value, PMCCNTR register
  asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(value) : );

  return value;
}

#endif // ZYNQ_PMM_USER__
