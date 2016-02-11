/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * %Id%
 *------------------------------------------------------------------------------
 * %Log%
 ******************************************************************************/

#include "raid.h"
#include "detect.h"
#include "release.h"
#include "lena.c"

#ifdef PULP
#include "zynq.h"
#include "pulp_host.h"
#include "pulp_func.h"
#include <time.h> // for time measurements
PulpDev * PULP_pulp;
#endif

int main(int argc, const char *argv[])
{

#ifdef PULP
  int ret;

  /*
   * Preparation
   */
  char app_name[15];
  int timeout_s = 10;  
  int pulp_clk_freq_mhz = 50;

  strcpy(app_name,"face_detect");

  if (argc>1)
    pulp_clk_freq_mhz = atoi(argv[1]);

  /*
   * Initialization
   */
  printf("PULP Initialization\n");
 
  PulpDev pulp_dev;
  PulpDev *pulp;
  pulp = &pulp_dev;

  PULP_pulp = pulp;

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
  printf("PULP Boot\n");
  // load binary
  pulp_load_bin(pulp,app_name);

  /*************************************************************************/
 
  // enable RAB miss handling
  pulp_rab_mh_enable(pulp);

  // start execution
  pulp_exe_start(pulp);
#endif

  const char *input = "lena4.raw";
  IMG_image_t inImage;
  detect_param_t param;
    
  param.rotation = 1;
  param.xStep = -1;
  param.yStep = -1;
  param.validScore = 5;
  param.iiStripeWidth = 32;
  param.scalingStep = 0.85;
  param.cascade = 0;    

  IMG_Create(&inImage,WIDTH,HEIGHT,IMG_TYPE_GREY,8);
    
  //Load Image
  int i,j;
  for (i = 0; i < WIDTH; ++i)
    for (j = 0; j < HEIGHT; ++j)
      inImage.data[i*WIDTH+j] = rowImage[i*WIDTH+j];
    
  printf("Processing fDetect\n");
  detect(&inImage, &param);

#ifdef PULP
  // stop execution
  pulp_exe_stop(pulp);

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
  
  printf("PULP Cleanup\n");
  pulp_rab_free(pulp,0);
  pulp_free_v_addr(pulp);
  sleep(1);
  pulp_munmap(pulp);
#endif
    
  /// Results:
  printf("%d faces detected in image: %s\n", param.list.NbRect, input);
  Rect_t * pRect = param.list.pHeadRect;
  int x, y, w, h;
  while(pRect != NULL){
    x = (pRect->x1-pRect->x0)/2+pRect->x0;
    y = (pRect->y1-pRect->y0)/2+pRect->y0;
    w = pRect->x1-pRect->x0;
    h = pRect->y1-pRect->y0;
    printf("Face:   x:%d, \ty:%d, \tw:%d, \th:%d, \tscore:%d\n", x, y, w, h, pRect->score);
    pRect = pRect->pNextRect;
  }

  return 0;
}
