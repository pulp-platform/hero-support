#ifndef __CT_H__
#define __CT_H__

#define PROFILE

typedef struct {
  int n_clusters;
  int w;
  int h;
  int Bh;
  uint8_t *input;
  unsigned int *mom;
  unsigned int cycles;

#ifdef PROFILE    
  unsigned int t_comp;
  unsigned int t_dma_in;
#endif
    
  unsigned int padding;
    
  DataDesc* data_desc;
  TaskDesc *desc;
#if MEM_SHARING == 1
  TaskDesc *fdesc;
#endif
  //ompOffload_desc_t *desc;
  //omp_offload_t *offload;
  //uint32_t foffload;
}CT_kernel_t;
    
#endif
