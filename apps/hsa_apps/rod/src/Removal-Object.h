#ifndef __ROD_H__
#define __ROD_H__

#define MAX_ROI     8
#define R           3   //block ray
#define TH       1023   //ncc threshold
#define CLASS_TH   20   //threshold for abandoned/removed object classification
#define EDGE_TH   100
#define AREA_TH    10   //For area-closing, minimum roi side
#define SIDE_TH    10
#define RGB         3

#define max(a, b) (((a)>(b))? (a): (b))

typedef struct _Rois {
  int nrois;
  int *X1;
  int *Y1;
  int *X2;
  int *Y2;
  int *isValid;
  int *classification;
} Rois;

// #define PROFILE

typedef struct {
  int initBg;
  unsigned int Fbg;
  int type;
  int n_clusters;
  int w;
  int h;
  uint8_t *fg;
  uint8_t *bg;
  uint8_t *out;
  int ws;
  unsigned long long cycles;
#ifdef PROFILE    
  unsigned long long t_comp;
  unsigned long long t_dma_in;
  unsigned long long t_dma_out;
  //     unsigned long long t_dma_wait_in;
  //     unsigned long long t_dma_wait_out;
#endif    
  int padding[2];
  // vogelpi
  //data_desc_t* data_desc;
  //ompOffload_desc_t *desc;
  //omp_offload_t *offload;
  //uint32_t foffload;
  DataDesc *data_desc;
  TaskDesc *desc;
#if MEM_SHARING == 1
  TaskDesc *fdesc;
#endif
}NCC_kernel_t;

void erode(uint8_t*, uint8_t*, int, int, int);
void labeling(uint8_t*, int, int, int *);
int compute_rois(uint8_t*, int, int , Rois *);
void classify_roi(uint8_t *, uint8_t*, uint8_t*, int, int, Rois *);
inline int isContour(uint8_t*, int, int, int);
    
#endif
