#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// vogelpi
#include "pulp_host.h"
#include "pulp_func.h"
#include "zynq_pmm_user.h"
#include <time.h> // for time measurements

#include <sys/time.h> // for time measurements

//#include "ompOffload.h"
#include "utils.h"

#include "Removal-Object.h"

//#define REPETITIONS 2
#define REPETITIONS 5
//#define PIPELINE

int NCC_init(NCC_kernel_t *, int, int);
inline void NCC_offload_out(NCC_kernel_t *nccInstance, uint8_t *frame, int offload_id);
inline int NCC_exe_start(NCC_kernel_t *nccInstance);
inline int NCC_exe_wait(NCC_kernel_t *nccInstance);
inline int NCC_offload_in(NCC_kernel_t *nccInstance, uint8_t *frame);
void NCC_destroy(NCC_kernel_t *);
void drawROI ( uint8_t *image, int type, int width, int height, int x_1, int y_1, int x_2, int y_2, int color[] );

double GetTimeMs()
{
    /* Linux */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double ret = 0.0f;
    ret = (double) tv.tv_usec;
    /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
    ret /= 1000;
    /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
    ret += (((double) (tv.tv_sec)) * 1000);
    return ret;
}

// vogelpi
PulpDev pulp_dev;
PulpDev *pulp;
char name[4];

// for time measurement
#define ACC_CTRL 0 
int accumulate_time(struct timespec *tp1, struct timespec *tp2,
		    double *duration, int ctrl);
struct timespec res, tp1, tp2, tp1_local, tp2_local;
double s_duration, s_duration1, s_duration2, s_duration3;

#ifdef ZYNQ_PMM
// for cache miss rate measurement
int *zynq_pmm_fd;
int zynq_pmm;
#endif
int ret;

int main(int argc, char *argv[]) {
  raw_image_data_t foreground;
  raw_image_data_t background;
#ifdef PIPELINE
  NCC_kernel_t NCC_instance[2];
#else
  NCC_kernel_t NCC_instance;
#endif
  int nclass = 0;
  Rois roi;
  uint8_t *tmp, *tmp2;
  int i;
  
  // used to boot PULP
  strcpy(name,"rod");
  TaskDesc task_desc;
  task_desc.name = &name[0];
  int pulp_clk_freq_mhz = 50;

  if (argc>1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  /*
   * Initialization
   */ 
  pulp = &pulp_dev;
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

  // measure the actual clock frequency
  pulp_clk_freq_mhz = pulp_clking_measure_freq(pulp);
  printf("PULP actually running @ %d MHz.\n",pulp_clk_freq_mhz);
  
  // clear stdout
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  // clear contiguous L3
  for (i = 0; i<L3_MEM_SIZE_B/4; i++)
    pulp_write32(pulp->l3_mem.v_addr,i*4,'b',0);

  printf("PULP Boot\n");
  pulp_boot(pulp,&task_desc);

#if (MEM_SHARING == 3)
  // enable RAB miss handling
  pulp_rab_mh_enable(pulp);
#endif

  // setup time measurement
  s_duration = 0, s_duration1 = 0, s_duration2 = 0, s_duration3 = 0;
  
#ifdef ZYNQ_PMM
  // setup cache miss rate measurement
  char proc_text[ZYNQ_PMM_PROC_N_CHARS_MAX];
  long long counter_values[(N_ARM_CORES+1)*2];
  double cache_miss_rates[(N_ARM_CORES+1)*2];
  zynq_pmm_fd = &zynq_pmm;
  ret = zynq_pmm_open(zynq_pmm_fd);
  if (ret)
    return ret;

  // delete the accumulation counters
  ret = zynq_pmm_parse(proc_text, counter_values, -1);
#endif

  // measure time
  clock_getres(CLOCK_REALTIME,&res);
  clock_gettime(CLOCK_REALTIME,&tp1);

  roi.nrois          = 0;
  roi.X1             = (int *)malloc((MAX_ROI+1) * sizeof(int));
  roi.Y1             = (int *)malloc((MAX_ROI+1) * sizeof(int));
  roi.X2             = (int *)malloc((MAX_ROI+1) * sizeof(int));
  roi.Y2             = (int *)malloc((MAX_ROI+1) * sizeof(int));
  roi.isValid        = (int *)malloc((MAX_ROI+1) * sizeof(int));
  roi.classification = (int *)malloc((MAX_ROI+1) * sizeof(int));
    
  printf("\n###########################################################################\n");
  printf("\n[APP ] Load input images\n");
  //readPgmOrPpm("samples/ippo_256x64.pgm", &background);
  //readPgmOrPpm("samples/ippo2_256x64.pgm", &foreground);
  readPgmOrPpm("samples/ippo_256x384.pgm", &background);
  readPgmOrPpm("samples/ippo2_256x384.pgm", &foreground);

  //printf("background.width  = %d\n", background.width);
  //printf("background.height = %d\n", background.height);    
  tmp  = (uint8_t *)malloc(sizeof(uint8_t)*background.width*background.height);

  tmp2 = (uint8_t *)malloc(sizeof(uint8_t)*background.width*background.height);
  memcpy(tmp2, foreground.data, sizeof(uint8_t)*background.width*background.height);

#ifndef PIPELINE

  NCC_init(&NCC_instance, background.width, background.height);
  memcpy(NCC_instance.bg, background.data, sizeof(uint8_t)*background.width*background.height);

  for (i = 0; i < REPETITIONS; ++i){
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Execute offload nbclusters %d\n", NCC_instance.n_clusters);
    
    // write command to make PULP continue
    pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',PULP_START);
 
    //memcpy(foreground.data, NCC_instance.out, sizeof(uint8_t)*background.width*background.height);
    //writePgmOrPpm("samples/test.pgm", &foreground);
    //printf("written\n");
    //memcpy(foreground.data, tmp2, sizeof(uint8_t)*background.width*background.height);
    //sleep(10);

    // offload out
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    NCC_offload_out(&NCC_instance, foreground.data, i);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration1,ACC_CTRL);

    // start
    ret = NCC_exe_start(&NCC_instance);
    if ( ret ) {
      printf("ERROR: Execution start failed. ret = %d\n",ret);
      break;
    }

    if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d scheduled\n", i);

    // wait
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    ret = NCC_exe_wait(&NCC_instance);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration3,ACC_CTRL);
    if ( ret ) {
      printf("ERROR: Execution wait failed. ret = %d\n",ret);
      break;
    }

    // offload in
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    ret = NCC_offload_in(&NCC_instance,  foreground.data);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration1,ACC_CTRL);
    if ( ret ) {
      printf("ERROR: Offload in failed. ret = %d\n",ret);
      break;
    } 
    
    if (DEBUG_LEVEL > 0) printf("\n###########################################################################\n");
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Offload %d terminated\n",i);    
        
    //#ifdef PROFILE
    //    printf("\n[APP ] Kernel Cycles %llu\n", NCC_instance.cycles);
    //    printf("\n[APP ] DMA IN Cycles %llu\n", NCC_instance.t_dma_in);
    //    printf("\n[APP ] DMA OUT Cycles %llu\n", NCC_instance.t_dma_out);
    //    printf("\n[APP ] EXE Cycles %llu\n", NCC_instance.t_comp);
    //    printf("\n###########################################################################\n");
    //#endif
#ifdef ZYNQ_PMM
    zynq_pmm_read(zynq_pmm_fd,proc_text); // reset cache counters
#endif
    clock_gettime(CLOCK_REALTIME,&tp1_local);

    if (DEBUG_LEVEL > 0) printf("[APP ] Erode %d\n",i);
    erode(NCC_instance.out, tmp, background.width, background.height, 2);

    if (DEBUG_LEVEL > 0) printf("\n[APP ] Labeling %d\n",i);
    labeling(NCC_instance.out, background.width, background.height, &nclass);
        
    if(nclass < MAX_ROI)
      roi.nrois = nclass;
    else
      roi.nrois = MAX_ROI;    
    if (DEBUG_LEVEL > 0) printf("roi.nrois = %d\n",roi.nrois);

    if (DEBUG_LEVEL > 0) printf("\n[APP ] Compute Region of Interest %d\n", i);
    compute_rois(NCC_instance.out, background.width, background.height, &roi);
        
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Classify Roi %d\n",i);
    //// change foreground.data
    //classify_roi(foreground.data, NCC_instance.out, foreground.data, background.width, background.height, &roi);
    // change tmp2
    classify_roi(tmp2, NCC_instance.out, foreground.data, background.width, background.height, &roi);

     if (DEBUG_LEVEL > 0) printf("\n###########################################################################\n");

    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration2,ACC_CTRL);
#ifdef ZYNQ_PMM 
    zynq_pmm_read(zynq_pmm_fd, proc_text);
    zynq_pmm_parse(proc_text, counter_values, 1); // accumulate cache counter values
#endif
    if (DEBUG_LEVEL > 2) {
      pulp_stdout_print(pulp,0);
      pulp_stdout_print(pulp,1);
      pulp_stdout_print(pulp,2);
      pulp_stdout_print(pulp,3);
    }
  }
  
  printf("\n[APP ] Write image output\n");
  // change tmp2
  memcpy(foreground.data, tmp2, sizeof(uint8_t)*background.width*background.height);
  writePgmOrPpm("samples/test_out.pgm", &foreground);
  printf("\n###########################################################################\n");

  NCC_destroy(&NCC_instance);

#else // PIPELINE

  NCC_init(&NCC_instance[0], background.width, background.height);
  memcpy(NCC_instance[0].bg, background.data, sizeof(uint8_t)*background.width*background.height);
  NCC_init(&NCC_instance[1], background.width, background.height);
  memcpy(NCC_instance[1].bg, background.data, sizeof(uint8_t)*background.width*background.height);

#ifdef PROFILE
  NCC_instance[0].n_clusters = 1;
  NCC_instance[1].n_clusters = 1;
#else        
  NCC_instance[0].n_clusters = i%4 + 1;
  NCC_instance[1].n_clusters = i%4 + 1;
#endif
    
  int buff_id = 0;
  int next_i = 0;
    
  if (DEBUG_LEVEL > 0) printf("\n[APP ] Execute offload nbclusters %i\n", NCC_instance[0].n_clusters);

  clock_gettime(CLOCK_REALTIME,&tp1_local);
  NCC_launch(&NCC_instance[buff_id], foreground.data);
  clock_gettime(CLOCK_REALTIME,&tp2_local);
  accumulate_time(&tp1_local,&tp2_local,&s_duration1,ACC_CTRL);   
    
  if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d scheduled\n", 0);

  for (i = 0; i < REPETITIONS; ++i){
        
    buff_id = (buff_id == 0) ? 1 : 0;
    next_i++;

    if (next_i < REPETITIONS) {

      if (DEBUG_LEVEL > 0) printf("\n[APP ] Execute offload nbclusters %d\n", NCC_instance[buff_id].n_clusters);

      clock_gettime(CLOCK_REALTIME,&tp1_local);
      NCC_launch(&NCC_instance[buff_id], foreground.data);
      clock_gettime(CLOCK_REALTIME,&tp2_local);
      accumulate_time(&tp1_local,&tp2_local,&s_duration1,ACC_CTRL);

      if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d scheduled\n", next_i);

    }
    NCC_wait(&NCC_instance[!buff_id]);
   
    if (DEBUG_LEVEL > 0) printf("\n###########################################################################\n");
    if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d terminated\n",i);    
#ifdef ZYNQ_PMM
    zynq_pmm_read(zynq_pmm_fd,proc_text); // reset cache counters    
#endif 
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    
    if (DEBUG_LEVEL > 0) printf("[APP ] Erode %d\n",i);
    erode(NCC_instance[!buff_id].out, tmp, background.width, background.height, 2);
        
    if (DEBUG_LEVEL > 0) printf("[APP ] Labeling %d\n",i);
    labeling(NCC_instance[!buff_id].out, background.width, background.height, &nclass);
        
    if(nclass < MAX_ROI)
      roi.nrois = nclass;
    else
      roi.nrois = MAX_ROI;
        
    if (DEBUG_LEVEL > 0) printf("[APP ] Compute Region of Interest %d\n", i);
    compute_rois(NCC_instance[!buff_id].out, background.width, background.height, &roi);
        
    if (DEBUG_LEVEL > 0) printf("[APP ] Classify Roi %d\n",i);
    classify_roi(foreground.data, NCC_instance[!buff_id].out, foreground.data, background.width, background.height, &roi);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration2,ACC_CTRL);
#ifdef ZYNQ_PMM  
    zynq_pmm_read(zynq_pmm_fd, proc_text);
    zynq_pmm_parse(proc_text, counter_values, 1); // accumulate cache counter values
#endif
  }
  printf("[APP ] Write image output\n");
  writePgmOrPpm("samples/test_out.pgm", &foreground);
  printf("\n###########################################################################\n");

  NCC_destroy(&NCC_instance[0]);
  NCC_destroy(&NCC_instance[1]);

#endif // PIPELINE

  free(background.data);
  free(foreground.data); 

  // measure time
  clock_gettime(CLOCK_REALTIME,&tp2);
  accumulate_time(&tp1,&tp2,&s_duration,ACC_CTRL);
  
  // print time measurements
   printf("\n###########################################################################\n");
  printf("Total Offload Time [s]:     %.6f\n",s_duration1);
  printf("Total Host Wait Time [s]:   %.6f\n",s_duration3);
  printf("Total Host Kernel Time [s]: %.6f\n",s_duration2);
  printf("Total Execution Time [s]:   %.6f\n",s_duration);
  printf("\n######################################################################\n");
  //double dma_time = (double)pulp_read32(pulp->mb_mem.v_addr,DMA_TIME_REG_OFFSET_B,'b')/(MB_CLK_FREQ_MHZ * 1000000);
  //printf("Total DMA Time \t\t : %.9f seconds\n", dma_time);
  //double pmca_time = (double)(REPETITIONS*N_STRIPES*KERNEL_CYCLES_PER_STRIPE)/(STHORM_CLK_FREQ_MHZ * 1000000);
  //printf("Total PMCA Kernel Time \t : %.9f seconds\n", pmca_time );
  //printf("\n######################################################################\n");
  //printf("Avg. Offload Time \t : %u.%09llu seconds\n",s_duration1/REPETITIONS,ns_duration1/REPETITIONS+(((unsigned long long)(s_duration1 % REPETITIONS)*1000000000) / REPETITIONS));
  //printf("Avg. Host Wait Time \t : %u.%09llu seconds\n",s_duration3/REPETITIONS,ns_duration3/REPETITIONS+(((unsigned long long)(s_duration3 % REPETITIONS)*1000000000) / REPETITIONS));
  //printf("Avg. Host Kernel Time \t : %u.%09llu seconds\n",s_duration2/REPETITIONS,ns_duration2/REPETITIONS+(((unsigned long long)(s_duration2 % REPETITIONS)*1000000000) / REPETITIONS));
  //printf("\n######################################################################\n");
  //double dma_bandwidth = (double)(REPETITIONS*N_STRIPES*(DMA_IN_SIZE_B+DMA_OUT_SIZE_B))/(dma_time*1000000);
  //printf("Achieved DMA Bandwidth \t : %.2f MiB/s\n",dma_bandwidth);
  //double dma_bw_util = dma_bandwidth/2400*100;
  //printf("DMA Bandwidth Utilization: %.2f %% \n",dma_bw_util);
  //printf("\n######################################################################\n");
#ifdef ZYNQ_PMM
  //zynq_pmm_compute_rates(cache_miss_rates, counter_values); // compute cache miss rates
  //double miss_rate_0 = cache_miss_rates[0]*100;
  //double miss_rate_1 = cache_miss_rates[1]*100;
  //printf("Host Kernel L1 D-Cache Miss Rates: %.2f %% (Core 0), %.2f %% (Core 1)\n",miss_rate_0,miss_rate_1);
  //printf("\n###########################################################################\n");
  //printf("MATLAB:\n");
  //printf(" [ %u.%09lu, %u.%09lu, %u.%09lu, %u.%09lu, %.9f, %.9f, %.2f, %.2f, %.2f, %.2f ];\n",s_duration1,ns_duration1,s_duration3,ns_duration3, s_duration2,ns_duration2, s_duration,ns_duration, dma_time, pmca_time, dma_bandwidth, dma_bw_util, miss_rate_0, miss_rate_1 );
#endif 
  // vogelpi
  //printf("Offload status 1 = %u \n",pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B, 'b') );
  //printf("Offload status 2 = %u \n",pulp_read32(pulp->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B, 'b') );
  //printf("DMA %i status = %#x\n",0,pulp_dma_status(pulp,0));
  //printf("DMA %i status = %#x\n",1,pulp_dma_status(pulp,1));

#if (MEM_SHARING == 3)
  // disable RAB miss handling
  pulp_rab_mh_disable(pulp);
#endif

 sleep(0.5);
 
  /*
   * Cleanup
   */
 sleep(0.5);
 pulp_stdout_print(pulp,0);
 pulp_stdout_print(pulp,1);
 pulp_stdout_print(pulp,2);
 pulp_stdout_print(pulp,3);
 pulp_stdout_clear(pulp,0);
 pulp_stdout_clear(pulp,1);
 pulp_stdout_clear(pulp,2);
 pulp_stdout_clear(pulp,3);
 
 sleep(1);

#ifdef ZYNQ_PMM  
 zynq_pmm_close(zynq_pmm_fd);
#endif

 pulp_rab_free_striped(pulp);
 pulp_rab_free(pulp,0);
 pulp_free_v_addr(pulp);
 sleep(1);
 pulp_munmap(pulp);
     
 return 0;
}

/*****************************************************************************************/

int NCC_init(NCC_kernel_t *nccInstance, int width, int height){
  nccInstance->w          = width;
  nccInstance->h          = height;
  nccInstance->fg         = 0x0;
  nccInstance->initBg     = 1;
  // vogelpi -- allocate memory in contigous L3
  //nccInstance->bg       = (uint8_t *)p2012_socmem_alloc(nccInstance->w*nccInstance->h*sizeof(uint8_t),
  //							&nccInstance->Fbg);
  //#if (MEM_SHARING == 1) 
  //nccInstance->bg         = (uint8_t *)pulp_l3_malloc(pulp, nccInstance->w*nccInstance->h*sizeof(uint8_t),
  //						      &nccInstance->Fbg);
  //#else // 2 --> allocate memory for background also for every offload in pulp_offload_out_contiguous
  nccInstance->bg         = (uint8_t *)malloc(nccInstance->w*nccInstance->h*sizeof(uint8_t));
  //#endif // MEM_SHARING  
  nccInstance->out        = (uint8_t *)malloc(nccInstance->w*nccInstance->h*sizeof(uint8_t));
#ifdef PROFILE
  nccInstance->data_desc  = (DataDesc *)malloc(9*sizeof(DataDesc));
#else
  nccInstance->data_desc  = (DataDesc *)malloc(6*sizeof(DataDesc));
#endif // PROFILE
  nccInstance->n_clusters = 1;
  return 0;
}

inline void NCC_offload_out(NCC_kernel_t *nccInstance, uint8_t *frame, int offload_id) {
  nccInstance->data_desc[0].ptr  = (void *) frame;
  nccInstance->data_desc[0].size = nccInstance->w*nccInstance->h*sizeof(uint8_t);
  nccInstance->data_desc[0].type = 1;
  //nccInstance->data_desc[1].ptr  = (void *) nccInstance->Fbg;
  //nccInstance->data_desc[1].size = nccInstance->w*nccInstance->h*sizeof(uint8_t);
  //nccInstance->data_desc[1].type = -1;
  nccInstance->data_desc[1].ptr  = (void *) nccInstance->bg;
  nccInstance->data_desc[1].size = nccInstance->w*nccInstance->h*sizeof(uint8_t);
  nccInstance->data_desc[1].type = 1;
  nccInstance->data_desc[2].ptr  = (void *) nccInstance->out;
  nccInstance->data_desc[2].size = nccInstance->w*nccInstance->h*sizeof(uint8_t);
  nccInstance->data_desc[2].type = 2;
  nccInstance->data_desc[3].ptr  = &nccInstance->w;
  nccInstance->data_desc[3].size = sizeof(int);
  nccInstance->data_desc[3].type = 1;
  nccInstance->data_desc[4].ptr  = &nccInstance->h;
  nccInstance->data_desc[4].size = sizeof(int);
  nccInstance->data_desc[4].type = 1;
  nccInstance->data_desc[5].ptr  = &nccInstance->cycles;
  nccInstance->data_desc[5].size = sizeof(unsigned long long);
  nccInstance->data_desc[5].type = 2;
#ifdef PROFILE    
  nccInstance->data_desc[6].ptr  = &nccInstance->t_comp;
  nccInstance->data_desc[6].size = sizeof(unsigned long long);
  nccInstance->data_desc[6].type = 2;
  nccInstance->data_desc[7].ptr  = &nccInstance->t_dma_in;
  nccInstance->data_desc[7].size = sizeof(unsigned long long);
  nccInstance->data_desc[7].type = 2;
  nccInstance->data_desc[8].ptr  = &nccInstance->t_dma_out;
  nccInstance->data_desc[8].size = sizeof(unsigned long long);
  nccInstance->data_desc[8].type = 2;
#endif // PROFILE
  nccInstance->desc              = (TaskDesc *)malloc(sizeof(TaskDesc));

#ifdef PROFILE
  nccInstance->desc->n_data      = 9;
#else
  nccInstance->desc->n_data      = 6;
#endif // PROFILE

  nccInstance->desc->data_desc   = nccInstance->data_desc;
  nccInstance->desc->n_clusters  = nccInstance->n_clusters;
  nccInstance->desc->name        = &name[0];
  nccInstance->desc->task_id     = offload_id;
  
#if (MEM_SHARING == 1)
  pulp_offload_out_contiguous(pulp, nccInstance->desc, &(nccInstance->fdesc));
#else // 2
  pulp_offload_out(pulp, nccInstance->desc);
#endif // MEM_SHARING
}

inline int NCC_exe_start(NCC_kernel_t *nccInstance) {
//inline void NCC_exe_start(NCC_kernel_t *nccInstance) {
  //pulp_offload_start(pulp, nccInstance->desc);
  //return;
  return pulp_offload_start(pulp, nccInstance->desc);
}

inline int NCC_exe_wait(NCC_kernel_t *nccInstance) {
  //inline void NCC_exe_wait(NCC_kernel_t *nccInstance) {
  //pulp_offload_wait(pulp, nccInstance->desc);
  //return;
  return pulp_offload_wait(pulp, nccInstance->desc);
}

inline int NCC_offload_in(NCC_kernel_t *nccInstance, uint8_t *frame) {
  //inline void NCC_offload_in(NCC_kernel_t *nccInstance, uint8_t *frame) {
  int ret;
#if (MEM_SHARING == 1)
  ret = pulp_offload_in_contiguous(pulp, nccInstance->desc, &nccInstance->fdesc);
#else // 2
  ret = pulp_offload_in(pulp, nccInstance->desc); 
#endif // MEM_SHARING
  free(nccInstance->desc);
  return ret;
}

void NCC_destroy(NCC_kernel_t *nccInstance){
  // vogelpi -- free contiguous L3 memory
  //p2012_socmem_free((uint32_t)nccInstance->bg, (uint32_t)nccInstance->Fbg);
  //acc_l3_free(acc, (uint32_t)nccInstance->bg, (uint32_t)nccInstance->Fbg);
#if (MEM_SHARING == 1)
  //free(nccInstance->fdesc);
#endif
  free(nccInstance->data_desc);
  free(nccInstance->out);
  free(nccInstance->bg);
}

//void NCC_wait(NCC_kernel_t *nccInstance){
//  // vogelpi -- wait for fake offload to finish
//  //GOMP_offload_wait(nccInstance->offload,nccInstance->foffload);
//  clock_gettime(CLOCK_REALTIME,&tp1_local);
//  acc_offload_wait(acc, nccInstance->offload, nccInstance->foffload);
//  clock_gettime(CLOCK_REALTIME,&tp2_local);
//  accumulate_time(&tp1_local,&tp2_local,&s_duration3,&ns_duration3,ACC_CTRL);
//
//  clock_gettime(CLOCK_REALTIME,&tp1_local);
//  acc_offload_finish(acc, nccInstance->offload, nccInstance->foffload);
//  clock_gettime(CLOCK_REALTIME,&tp2_local);
//  accumulate_time(&tp1_local,&tp2_local,&s_duration1,&ns_duration1,ACC_CTRL);
//
//  free(nccInstance->desc);
//}

void erode(uint8_t* src, uint8_t* dst, int w, int h, int iter){
  int i,j,n;
    
  for(n=0; n<iter; n++)
    {
      for(i=R; i<h-R; i++)    
	for(j=R; j<w-R; j++)
	  if(src[i*w+j]==255 && isContour(src, j, i, w) ) 
	    dst[i*w+j]=0;
	  else
	    dst[i*w+j]=src[i*w+j];
                
      if(n != iter-1)
	{
	  uint8_t *temp = src;
	  src = dst;
	  dst = temp; 
	}
    }
}

void labeling(uint8_t* I, int w, int h, int *nclass){
  int i,j,k;
  int Nlabel = 4800;
    
  uint8_t F = 255;
  uint8_t B = 0;
  uint8_t lp, lq, lx = 0;
    
  int NewLabel = 0;
    
  int *C = (int *)malloc(Nlabel * sizeof(int));
  //int *C = malloc(Nlabel * sizeof(int));
    
  for(i=0; i<Nlabel; i++)
    C[i] = i;
    
  // FIRST SCAN
  for(i=R; i<h-R; i++)
    for(j=R; j<w-R; j++){
      if (I[i*w + j] == F){
	lp = I[(i-1)*w  + j];
	lq = I[i*w  + j-1];
                
	// If no pixel before this with pixel -> New Label
	if(lp == B && lq == B){
	  NewLabel++;
	  lx = NewLabel;
	}
	else if((C[lp]!=C[lq])&&(lp != B)&&(lq != B))
	  {
	    int C_lp = C[lp];
	    for(k=1; k<=NewLabel; k++)
	      if (C[k] == C_lp)
		C[k]=C[lq];
	    lx = lq;
	  }
	else if(lq != B)
	  lx = lq;
	else if(lp != B)
	  lx = lp;
                
	I[i*w+j] = lx;
      }
    }
        
  // SECOND SCAN
  for(i=R; i<h-R; i++)
    for(j=R; j<w-R; j++)
      if (I[i*w+j] != B) 
	I[i*w+j] = C[I[i*w+j]];
            
  //NORMALIZE CLASSES
  int count = 1;
  int *FC = C; FC[0] = 0;
  for(i=R; i<h-R; i++)
    for(j=R; j<w-R; j++)
      if (I[i*w+j] != B)
	{
	  int k;
	  for(k=0; k<count; k++)
	    if( FC[k] == I[i*w+j])
	      {
		I[i*w+j] = k;
		goto label_skip;
	      }
                    
	  //New class:
	  FC[count] = I[i*w+j];
	  I[i*w+j] = count;
	  count++;
                
	label_skip: continue;
	}
                
  *nclass = count-1;
  free(C);
}

int compute_rois(uint8_t* I, int w, int h, Rois *roi){
  int i,j;
    
  for(i=0; i<roi->nrois+1; i++){
    roi->X1[i] = w;
    roi->Y1[i] = h;
    roi->X2[i] = 0;
    roi->Y2[i] = 0;
  }
    
  for(i=R; i<h-R; i++)
    for(j=R; j<w-R; j++)
      if (I[i*w+j] != 0) {
	if(j < roi->X1[I[i*w+j]])
	  roi->X1[I[i*w+j]] = j;
                
	if(i < roi->Y1[I[i*w+j]])
	  roi->Y1[I[i*w+j]] = i;
                
	if(j > roi->X2[I[i*w+j]])
	  roi->X2[I[i*w+j]] = j;
                
	if(i > roi->Y2[I[i*w+j]])
	  roi->Y2[I[i*w+j]] = i;
      }
            
  //AREA-CLOSING
  int n_valid_rois=0;
  for(i=1; i<=roi->nrois; i++){
    if( (abs(roi->X1[i]-roi->X2[i]) < SIDE_TH || abs(roi->Y1[i]-roi->Y2[i]) < SIDE_TH))
      roi->isValid[i] = 0;
    else{
      roi->isValid[i] = 1;
      n_valid_rois++;
    }
  }
  return n_valid_rois;
}


void classify_roi(uint8_t *image, uint8_t* C, uint8_t* I, int w, int h, Rois *roi)
{
  int i,j,n;
  int red[3]  = {255, 0, 0};
  int blue[3] = {0, 0, 255};
    
  for(n=1; n<=roi->nrois; n++)
    if(roi->isValid[n]){
      int cont_count = 0;
      int sum_d = 0;
            
      for(j=roi->Y1[n]; j<roi->Y2[n]; j++)
	for(i=roi->X1[n]; i<roi->X2[n]; i++){
	  if ( C[j*w+i] != 0 && isContour(C, i, j, w) ){
                        
	    cont_count++;
                        
	    int Dh = I[(j-1)*w+i-1] + 2* I[j*w+i-1] + I[(j+1)*w+i-1] - I[(j-1)*w+i+1] - 2*I[j*w+i+1] - I[(j+1)*w+i+1];
	    int Dv = I[(j-1)*w+i-1] + 2* I[(j-1)*w+i] + I[(j-1)*w+i+1] - I[(j+1)*w+i-1] -2*I[(j+1)*w+i] - I[(j+1)*w+i+1];
                        
	    if( max(abs(Dh), abs(Dv) ) > EDGE_TH)
	      sum_d++; 
	  }
	}
                
      if(cont_count != 0)
	sum_d = (sum_d*100)/cont_count;
            
      if(sum_d > CLASS_TH){
	roi->classification[n] = 1; //ABBANDONATO
	if (DEBUG_LEVEL > 0) printf("[APP ] ABANDONED item at %d,%d\n", roi->Y1[n], roi->X1[n]);       
	drawROI (image, 0, w, h, roi->X1[n], roi->Y1[n], roi->X2[n], roi->Y2[n], red );
                
      }
      else{
	roi->classification[n] = 0; //RIMOSSO
	if (DEBUG_LEVEL > 0) printf("[APP ] REMOVED item at %d,%d\n", roi->Y1[n], roi->X1[n]);
	drawROI (image, 0, w, h, roi->X1[n], roi->Y1[n], roi->X2[n], roi->Y2[n], blue );
      }
    }//nrois + isValid
}

inline int isContour(uint8_t* I, int x, int y, int w){
  int i, j;

  for (j=-1; j<=1; j++)
    for(i=-1; i<=1; i++)
      if(I[(y+j)*w+x+i] == 0)
	return 1;
            
  return 0;
}

void drawROI ( uint8_t *image, int type, int width, int height, int x_1, int y_1, int x_2, int y_2, int color[] ){
    
  int i, j, e;
  int r=color[0], g=color[1], b=color[2];
  int gray = (uint8_t)(((float)(r)*0.1140f) + ((float)(g)*0.5870f) + ((float)(b)*0.2983f));
    
  int ep = 2;
    
  for (i=x_1; i<=x_2; i++){
    if (type == RGB){
      for (e=-ep;e<=ep;e++){
	if (y_1+e>=0){
	  image[((y_1+e)*width+i)*3] = r;
	  image[((y_1+e)*width+i)*3+1] = g;
	  image[((y_1+e)*width+i)*3+2] = b;
	}
	if (y_2+e<height){
	  image[((y_2+e)*width+i)*3] = r;
	  image[((y_2+e)*width+i)*3+1] = g;
	  image[((y_2+e)*width+i)*3+2] = b;
	}
      }
    }
    else {
      for (e=-ep;e<=ep;e++){
	if (y_1+e>=0){
	  image[(y_1+e)*width+i] = gray;
	}
	if (y_2+e<height){
	  image[(y_2+e)*width+i] = gray;
	}
      }
    }
  }
  for (j=y_1; j<=y_2; j++){
    if (type == RGB){
      for (e=-ep;e<=ep;e++){
	if (x_1+e>=0){
	  image[(j*width+x_1+e)*3] = r;
	  image[(j*width+x_1+e)*3+1] = g;
	  image[(j*width+x_1+e)*3+2] = b;
	}
	if (x_2+e<width){
	  image[(j*width+x_2+e)*3] = r;
	  image[(j*width+x_2+e)*3+1] = g;
	  image[(j*width+x_2+e)*3+2] = b;
	}
      }
    }
    else {
      for (e=-ep;e<=ep;e++){
	if (x_1+e>=0){
	  image[j*width+x_1+e] = gray;
	}
	if (x_2+e<width){
	  image[j*width+x_2+e] = gray;
	}
      }
    }
  }
}

int accumulate_time(struct timespec *tp1, struct timespec *tp2, double *duration, int ctrl) {

  double start, end, tmp_duration;
  
  // compute and output the measured time
  start = ((double)(tp1->tv_sec))*1000000000 + (double)(tp1->tv_nsec); 
  end = ((double)(tp2->tv_sec))*1000000000 + (double)(tp2->tv_nsec); 
  tmp_duration = (end - start)/1000000000;

  if (ctrl == 1) {
    printf("Elapsed time [s] = %.6f\n",tmp_duration);
  }

  *duration += tmp_duration;

  if (ctrl == 1) {
    printf("Total elapsed time [s] = %.6f\n",*duration);
  }

  return 0;
}
