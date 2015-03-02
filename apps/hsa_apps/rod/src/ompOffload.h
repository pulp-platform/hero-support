#ifndef __OMP_OFFLOAD__
#define __OMP_OFFLOAD__

#include "ld/p2012_lnx_dd_host.h"
#include "ocld.h"
#include "pthread.h"
#include "omp-common.h"

/* Omp Termination Control */
typedef struct omp_termination_s
{
    uint32_t value;
    struct omp_termination_s *next;
    struct omp_termination_s *prev;
} omp_termination_t;

extern omp_termination_t *omp_terminated;

void ompMsgCallback(unsigned int msg);

int sthorm_omp_rt_init();
int GOMP_offload_wait(omp_offload_t *omp_offload, uint32_t fomp_offload);
int GOMP_offload_nowait(ompOffload_desc_t *desc, omp_offload_t **omp_offload, uint32_t *fomp_offload);
inline int gomp_offload_enqueue(uint32_t *fomp_offload);
inline int gomp_offload_wait(omp_offload_t *omp_offload, uint32_t fomp_offload);
inline int gomp_offload_datacpy_in(omp_offload_t *omp_offload, uint32_t fomp_offload);
inline int gomp_offload_datacpy_out(ompOffload_desc_t *desc, omp_offload_t **omp_offload, uint32_t *fomp_offload);
#endif
