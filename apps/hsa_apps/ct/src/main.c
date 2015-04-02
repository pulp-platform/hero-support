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

#define PROFILE

//#include "ompOffload.h"
#include "utils.h"

#include "CT.h"

#define PULP_CLK_FREQ_MHZ 75
//#define PULP_CLK_FREQ_MHZ 65
//#define REPETITIONS 2
//#define REPETITIONS 10
#define REPETITIONS 50
//#define PIPELINE

int  CT_init   (CT_kernel_t *, int, int);
inline void CT_offload_out(CT_kernel_t *ctInstance, uint8_t *frame, int offload_id);
//inline void CT_exe_start(CT_kernel_t *ctInstance);
inline int CT_exe_start(CT_kernel_t *ctInstance);
//inline void CT_exe_wait(CT_kernel_t *ctInstance);
inline int CT_exe_wait(CT_kernel_t *ctInstance);
//inline void CT_offload_in(CT_kernel_t *ctInstance);
inline int CT_offload_in(CT_kernel_t *ctInstance);
void CT_destroy(CT_kernel_t *);

inline void plot_pixel(raw_image_data_t *image, int x, int y, int color)
{
  image->data[image->width*y*image->nr_channels+x] = color; 
  image->data[image->width*y*image->nr_channels+x+1] = color;
  image->data[image->width*y*image->nr_channels+x+2] = color;
}

#define sgn(x) ((x<0)?-1:((x>0)?1:0))
void line(raw_image_data_t *image, int x1, int y1, int x2, int y2, int color)
{
  int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;
    
  dx=x2-x1;      /* the horizontal distance of the line */
  dy=y2-y1;      /* the vertical distance of the line */
  dxabs=abs(dx);
  dyabs=abs(dy);
  sdx=sgn(dx);
  sdy=sgn(dy);
  x=dyabs>>1;
  y=dxabs>>1;
  px=x1;
  py=y1;
        
  if (dxabs>=dyabs) /* the line is more horizontal than vertical */
    {
      for(i=0;i<dxabs;i++)
        {
	  y+=dyabs;
	  if (y>=dxabs)
            {
	      y-=dxabs;
	      py+=sdy;
            }
	  px+=sdx;
	  plot_pixel(image,px,py,color);
        }
    }
  else /* the line is more vertical than horizontal */
    {
      for(i=0;i<dyabs;i++)
        {
	  x+=dxabs;
	  if (x>=dyabs)
            {
	      x-=dyabs;
	      px+=sdx;
            }
	  py+=sdy;
	  plot_pixel(image,px,py,color);
        }
    }
}

int merge (raw_image_data_t *A, raw_image_data_t *B, raw_image_data_t *C)
{
  int pixels = A->width*A->height;
  int i;
  for(i = 0; i < pixels; ++i)
    {
      if(A->data[i*A->nr_channels] != 0x0 ){
	C->data[i*C->nr_channels + 0] = A->data[i*A->nr_channels + 0];
	C->data[i*C->nr_channels + 1] = A->data[i*A->nr_channels + 1]; 
	C->data[i*C->nr_channels + 2] = A->data[i*A->nr_channels + 2];
      }    
      else {
	C->data[i*C->nr_channels + 0] = B->data[i*B->nr_channels + 0];
	C->data[i*C->nr_channels + 1] = B->data[i*B->nr_channels + 1];
	C->data[i*C->nr_channels + 2] = B->data[i*B->nr_channels + 2];
      }
    }
  return 0;
}

// vogelpi
PulpDev pulp_dev;
PulpDev *pulp;

// for time measurement
#define ACC_CTRL 0 
int accumulate_time(struct timespec *tp1, struct timespec *tp2,
		    unsigned *seconds, unsigned long *nanoseconds,
		    int ctrl);
struct timespec res, tp1, tp2, tp1_local, tp2_local;
unsigned s_duration, s_duration1, s_duration2, s_duration3;
unsigned long ns_duration, ns_duration1, ns_duration2, ns_duration3;

#ifdef ZYNQ_PMM
// for cache miss rate measurement
int *zynq_pmm_fd;
int zynq_pmm, ret;
#endif

int main(int argc, char *argv[]) {
  raw_image_data_t inputImage;
  raw_image_data_t output;
  raw_image_data_t history;
  int init = 0;
  int current_x = 0;
  int current_y = 0;
  double moment01, moment10, area;
  int next_x, next_y;
  int i;
#ifdef PIPELINE    
  CT_kernel_t CT_instance[2];
#else
  CT_kernel_t CT_instance;
#endif
  int error = 0;
  
  // used to boot PULP
  char name[4];
  strcpy(name,"ct");
  TaskDesc task_desc;
  task_desc.name = &name[0];

  /* Init the runtime */
  //sthorm_omp_rt_init();
  // vogelpi
  /*
   * Initialization
   */ 
  pulp = &pulp_dev;
  pulp_reserve_v_addr(pulp);
  pulp_mmap(pulp);
  //pulp_print_v_addr(pulp);
  pulp_reset(pulp);
  printf("PULP running at %d MHz\n",pulp_clking_set_freq(pulp,PULP_CLK_FREQ_MHZ));
  pulp_rab_free(pulp,0x0);
  pulp_init(pulp);
  
  pulp_stdout_clear(pulp,0);
  pulp_stdout_clear(pulp,1);
  pulp_stdout_clear(pulp,2);
  pulp_stdout_clear(pulp,3);

  pulp_boot(pulp,&task_desc);

  // setup time measurement
  s_duration = 0, s_duration1 = 0, s_duration2 = 0, s_duration3 = 0;
  ns_duration = 0, ns_duration1 = 0, ns_duration2 = 0, ns_duration3 = 0;

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
    
  printf("\n###########################################################################\n");
  printf("\n[APP ] Load input images\n");
  //readPgmOrPpm("samples/images/image.ppm", &inputImage);
  readPgmOrPpm("samples/images/image_512x384.ppm", &inputImage);
    
  output.data        = (uint8_t *)malloc(inputImage.width*inputImage.height*inputImage.nr_channels*sizeof(uint8_t));
  output.width       = inputImage.width;
  output.height      = inputImage.height;
  output.nr_channels = inputImage.nr_channels;
    
  history.data        = (uint8_t *)malloc(inputImage.width*inputImage.height*inputImage.nr_channels*sizeof(uint8_t));
  history.width       = inputImage.width;
  history.height      = inputImage.height;
  history.nr_channels = inputImage.nr_channels;
  memset(history.data,0x0,inputImage.width*inputImage.height*inputImage.nr_channels*sizeof(uint8_t));

#ifndef PIPELINE
    
  CT_init(&CT_instance, inputImage.width, inputImage.height);
    
  for (i = 0; i < REPETITIONS; ++i){        
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Execute offload nbclusters %d\n", CT_instance.n_clusters);

    // write command to make PULP continue
    pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',PULP_START);
    //pulp_write32(pulp->mailbox.v_addr,MAILBOX_WRDATA_OFFSET_B,'b',PULP_STOP);

    // offload
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    CT_offload_out(&CT_instance, inputImage.data,i);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration1,&ns_duration1,ACC_CTRL);
    
    // start
    ret = CT_exe_start(&CT_instance);
    if ( ret ) {
      printf("ERROR: Execution start failed. ret = %d\n",ret);
      error = 1;
      break;
    }
   
    if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d scheduled\n", i);
 
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    ret = CT_exe_wait(&CT_instance);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration3,&ns_duration3,ACC_CTRL);
    if ( ret ) {
      printf("ERROR: Execution wait failed. ret = %d\n",ret);
      error = 1;
      break;
    }
  
    clock_gettime(CLOCK_REALTIME,&tp1_local);
    ret = CT_offload_in(&CT_instance);
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration1,&ns_duration1,ACC_CTRL);
    if ( ret ) {
      printf("ERROR: Offload in failed. ret = %d\n",ret);
      error = 1;
      break;
    } 

    if (DEBUG_LEVEL > 0) printf("\n###########################################################################\n");
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Offload %d terminated\n",i);    
        
    //#ifdef PROFILE
    //printf("\n[APP ] Kernel Cycles %d\n", CT_instance.cycles);
    //printf("\n[APP ] DMA IN Cycles %d\n", CT_instance.t_dma_in);
    //printf("\n[APP ] EXE Cycles %d\n", CT_instance.t_comp);
    //printf("\n###########################################################################\n");
    //#endif
#ifdef ZYNQ_PMM
    zynq_pmm_read(zynq_pmm_fd,proc_text); // reset cache counters
#endif
    clock_gettime(CLOCK_REALTIME,&tp1_local);    

    moment01 = CT_instance.mom[1];
    moment10 = CT_instance.mom[2];
    area     = CT_instance.mom[0];
    next_x   = moment10/area;
    next_y   = moment01/area;
        
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Color at (%d,%d) (%f, %f, %f)\n", next_y, next_x, area, moment01, moment10);
        
    if(!init) {
      current_x  = next_x;
      current_y  = next_y;
      init = 1;
    }

    line(&history, current_x, current_y, next_x, next_y, 255);
        
    current_x  = next_x;
    current_y  = next_y;
        
    merge(&history,&inputImage,&output);
        
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration2,&ns_duration2,ACC_CTRL);
#ifdef ZYNQ_PMM  
    zynq_pmm_read(zynq_pmm_fd, proc_text);
    zynq_pmm_parse(proc_text, counter_values, 1); // accumulate cache counter values
#endif

if (DEBUG_LEVEL > 0)  {
    pulp_stdout_print(pulp,0);
    pulp_stdout_print(pulp,1);
    pulp_stdout_print(pulp,2);
    pulp_stdout_print(pulp,3);
}
    //sleep(20);
  }
  printf("\n###########################################################################\n");
    
  CT_destroy(&CT_instance);

#else // PIPELINE

  CT_init(&CT_instance[0], inputImage.width, inputImage.height);
  CT_init(&CT_instance[1], inputImage.width, inputImage.height);
    
  int buff_id = 0;
  int next_i = 0;

  if (DEBUG_LEVEL > 0) printf("\n[APP ] Execute offload nbclusters %d\n", CT_instance[0].n_clusters);

  clock_gettime(CLOCK_REALTIME,&tp1_local);
  CT_launch(&CT_instance[buff_id], inputImage.data);
  clock_gettime(CLOCK_REALTIME,&tp2_local);
  accumulate_time(&tp1_local,&tp2_local,&s_duration1,&ns_duration1,ACC_CTRL);

  if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d scheduled\n", 0);

  for (i = 0; i < REPETITIONS; ++i){

    buff_id = (buff_id == 0) ? 1 : 0;
    next_i++;
    
    if (next_i < REPETITIONS) {

      if (DEBUG_LEVEL > 0) printf("\n[APP ] Execute offload nbclusters %d\n", CT_instance[buff_id].n_clusters);

      clock_gettime(CLOCK_REALTIME,&tp1_local);
      CT_launch(&CT_instance[buff_id], inputImage.data);
      clock_gettime(CLOCK_REALTIME,&tp2_local);
      accumulate_time(&tp1_local,&tp2_local,&s_duration1,&ns_duration1,ACC_CTRL);

      if (DEBUG_LEVEL > 0) printf("[APP ] Offload %d scheduled\n", next_i);
    }
    CT_wait(&CT_instance[!buff_id]);
    
    if (DEBUG_LEVEL > 0) printf("\n###########################################################################\n");
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Offload %d terminated\n", i);    
        
    //#ifdef PROFILE
    //printf("\n[APP ] Kernel Cycles %d\n", CT_instance.cycles);
    //printf("\n[APP ] DMA IN Cycles %d\n", CT_instance.t_dma_in);
    //printf("\n[APP ] EXE Cycles %d\n", CT_instance.t_comp);
    //printf("\n###########################################################################\n");
    //#endif

    zynq_pmm_read(zynq_pmm_fd,proc_text); // reset cache counters    

    clock_gettime(CLOCK_REALTIME,&tp1_local);    

    moment01 = CT_instance[!buff_id].mom[1];
    moment10 = CT_instance[!buff_id].mom[2];
    area     = CT_instance[!buff_id].mom[0];
    next_x   = moment10/area;
    next_y   = moment01/area;
        
    if (DEBUG_LEVEL > 0) printf("\n[APP ] Color at (%d,%d) (%f, %f, %f)\n", next_y, next_x, area, moment01, moment10);
        
    if(!init) {
      current_x  = next_x;
      current_y  = next_y;
      init = 1;
    }

    line(&history, current_x, current_y, next_x, next_y, 255);
        
    current_x  = next_x;
    current_y  = next_y;
        
    merge(&history,&inputImage,&output);
        
    clock_gettime(CLOCK_REALTIME,&tp2_local);
    accumulate_time(&tp1_local,&tp2_local,&s_duration2,&ns_duration2,ACC_CTRL);

    zynq_pmm_read(zynq_pmm_fd, proc_text);
    zynq_pmm_parse(proc_text, counter_values, 1); // accumulate cache counter values

  }
  printf("\n###########################################################################\n");
    
  CT_destroy(&CT_instance[0]);
  CT_destroy(&CT_instance[1]);

#endif // PIPELINE

  free(inputImage.data);

  // measure time
  clock_gettime(CLOCK_REALTIME,&tp2);
  accumulate_time(&tp1,&tp2,&s_duration,&ns_duration,ACC_CTRL);

  // print time measurements
  printf("\n###########################################################################\n");
  printf("Total Offload Time \t : %u.%09lu seconds\n",s_duration1,ns_duration1);
  printf("Total Host Wait Time \t : %u.%09lu seconds\n",s_duration3,ns_duration3);
  printf("Total Host Kernel Time \t : %u.%09lu seconds\n",s_duration2,ns_duration2);
  printf("Total Execution Time \t : %u.%09lu seconds\n",s_duration,ns_duration);
  printf("\n######################################################################\n");
  //double dma_time = (double)acc_read32(acc->mb_mem.v_addr,DMA_TIME_REG_OFFSET_B,'b')/(MB_CLK_FREQ_MHZ * 1000000);
  //printf("Total DMA Time \t\t : %.9f seconds\n", dma_time);
  //double pmca_time = (double)(REPETITIONS*N_STRIPES*KERNEL_CYCLES_PER_STRIPE)/(STHORM_CLK_FREQ_MHZ * 1000000);
  //printf("Total PMCA Kernel Time \t : %.9f seconds\n", pmca_time );
  //printf("\n######################################################################\n");
  ////printf("Avg. Offload Time \t : %u.%09llu seconds\n",s_duration1/REPETITIONS,ns_duration1/REPETITIONS+(((unsigned long long)(s_duration1 % REPETITIONS)*1000000000) / REPETITIONS));
  ////printf("Avg. Host Wait Time \t : %u.%09llu seconds\n",s_duration3/REPETITIONS,ns_duration3/REPETITIONS+(((unsigned long long)(s_duration3 % REPETITIONS)*1000000000) / REPETITIONS));
  ////printf("Avg. Host Kernel Time \t : %u.%09llu seconds\n",s_duration2/REPETITIONS,ns_duration2/REPETITIONS+(((unsigned long long)(s_duration2 % REPETITIONS)*1000000000) / REPETITIONS));
  ////printf("\n######################################################################\n");
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
  //printf("Offload status 1 = %u \t",acc_read32(acc->mb_mem.v_addr, OFFLOAD_STATUS_1_REG_OFFSET_B, 'b') );
  //printf("Offload status 2 = %u \n",acc_read32(acc->mb_mem.v_addr, OFFLOAD_STATUS_2_REG_OFFSET_B, 'b') );
  //printf("DMA %i status = %#x\n",0,acc_dma_status(acc,0));
  //printf("DMA %i status = %#x\n",1,acc_dma_status(acc,1));
  //acc_reset(acc);
    
  sleep(0.5);
    
  /*
   * Cleanup
   */
 cleanup:
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

int CT_init(CT_kernel_t *ctInstance, int width, int height){
  ctInstance->w          = width;
  ctInstance->h          = height;
  //ctInstance->Bh         = 10;
  ctInstance->Bh         = 8;
  //ctInstance->Bh         = 6;
  ctInstance->input      = NULL;
  ctInstance->mom        = (unsigned int *)malloc(3*sizeof(unsigned int));
  ctInstance->cycles        = 0;

#ifdef PROFILE   
  ctInstance->data_desc  = (DataDesc *)malloc(8*sizeof(DataDesc));
#else
  ctInstance->data_desc  = (DataDesc *)malloc(6*sizeof(DataDesc));
#endif // PROFILE

  ctInstance->n_clusters = 1;
  return 0;
}

void CT_offload_out(CT_kernel_t *ctInstance, uint8_t *frame, int offload_id) {
  ctInstance->data_desc[0].ptr  = (void *) frame;
  ctInstance->data_desc[0].size = ctInstance->w*ctInstance->h*sizeof(uint8_t)*3;
  ctInstance->data_desc[0].type = 1;
  ctInstance->data_desc[1].ptr  = &ctInstance->w;
  ctInstance->data_desc[1].size = sizeof(int);
  ctInstance->data_desc[1].type = 1;
  ctInstance->data_desc[2].ptr  = &ctInstance->h;
  ctInstance->data_desc[2].size = sizeof(int);
  ctInstance->data_desc[2].type = 1;
  ctInstance->data_desc[3].ptr  = &ctInstance->Bh;
  ctInstance->data_desc[3].size = sizeof(int);
  ctInstance->data_desc[3].type = 1;
  ctInstance->data_desc[4].ptr  = ctInstance->mom;
  ctInstance->data_desc[4].size = 3*sizeof(unsigned int);
  ctInstance->data_desc[4].type = 2;
  ctInstance->data_desc[5].ptr  = &ctInstance->cycles;
  ctInstance->data_desc[5].size = sizeof(unsigned int);
  ctInstance->data_desc[5].type = 2;
#ifdef PROFILE    
  ctInstance->data_desc[6].ptr  = &ctInstance->t_comp;
  ctInstance->data_desc[6].size = sizeof(unsigned int);
  ctInstance->data_desc[6].type = 2;
  ctInstance->data_desc[7].ptr  = &ctInstance->t_dma_in;
  ctInstance->data_desc[7].size = sizeof(unsigned int);
  ctInstance->data_desc[7].type = 2;
#endif // PROFILE
    
  ctInstance->desc              = (TaskDesc *)malloc(sizeof(TaskDesc));

#ifdef PROFILE
  ctInstance->desc->n_data      = 8;
#else
  ctInstance->desc->n_data      = 6;
#endif // PROFILE
    
  ctInstance->desc->data_desc   = ctInstance->data_desc;
  ctInstance->desc->n_clusters  = ctInstance->n_clusters;
  ctInstance->desc->name        = (void *) 4;
    
#if (MEM_SHARING == 1)
  pulp_offload_out_contiguous(pulp, ctInstance->desc, &ctInstance->fdesc);
#else // 2
  pulp_offload_out(pulp, ctInstance->desc);
#endif // MEM_SHARING
}

inline int CT_exe_start(CT_kernel_t *ctInstance) {
//inline void CT_exe_start(CT_kernel_t *ctInstance) {
  //pulp_offload_start(pulp, ctInstance->desc);
  //return;
  return pulp_offload_start(pulp, ctInstance->desc);
}

inline int CT_exe_wait(CT_kernel_t *ctInstance) {
  //inline void CT_exe_wait(CT_kernel_t *ctInstance) {
  //pulp_offload_wait(pulp, ctInstance->desc);
  //return;
  return pulp_offload_wait(pulp, ctInstance->desc);
}

inline int CT_offload_in(CT_kernel_t *ctInstance) {
  //inline void CT_offload_in(CT_kernel_t *ctInstance) {
  int ret;
#if (MEM_SHARING == 1)
  ret = pulp_offload_in_contiguous(pulp, ctInstance->desc, &ctInstance->fdesc);
#else // 2
  ret = pulp_offload_in(pulp, ctInstance->desc); 
#endif // MEM_SHARING
  free(ctInstance->desc);
  return ret;
}

void CT_destroy(CT_kernel_t *ctInstance){
  free(ctInstance->data_desc);
  free(ctInstance->mom);
}

int accumulate_time(struct timespec *tp1, struct timespec *tp2, unsigned *seconds, unsigned long *nanoseconds, int ctrl) {

  unsigned tmp_s;
  unsigned long tmp_ns; 

  // compute and output the measured time
  tmp_s = (int)(tp2->tv_sec - tp1->tv_sec);
  if (tp2->tv_nsec > tp1->tv_nsec) { // no overflow
    tmp_ns = (tp2->tv_nsec - tp1->tv_nsec);
  }
  else {//(tp2.tv_nsec < tp1.tv_nsec) {// overflow of tv_nsec
    if (ctrl == 1)
      printf("tp2->tv_nsec < tp1->tv_nsec \n");
    tmp_ns = (1000000000 - tp1->tv_nsec + tp2->tv_nsec) % 1000000000;
    tmp_s -= 1;
  }

  if (ctrl == 1) {
    printf("Elapsed time in seconds = %i\n",tmp_s);
    printf("Elapsed time in nanoseconds = %09li\n",tmp_ns);
  }

  *seconds += tmp_s;
  *nanoseconds += tmp_ns;

  *seconds += (*nanoseconds / 1000000000);
  *nanoseconds = (*nanoseconds % 1000000000);
  
  if (ctrl == 1) {
    printf("Total elapsed time in seconds = %i\n",*seconds);
    printf("Total elapsed time in nanoseconds = %09li\n",*nanoseconds);
  }

  return 0;
}
