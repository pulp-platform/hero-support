/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: detect_core.c.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: detect_core.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include <raid.h>
#include "detect.h"
#include "integral.h"
#include "detect_core.h"
#include "sqrt64.h"

#ifdef PULP
#include "pulp_func.h"
#include "time.h"
#include "zynq_pmm_user.h"
extern PulpDev* PULP_pulp;
#endif

#define DEBUG_LEVEL 1

//#define DEBUG_CASCADE
#define LIIW_SCAN_CENTERED 1

//-----------------------------------------------------------------------------
//! \brief  calculate statistics and  the normalize value of
//!     the patch of the integral image
//!
//! \param integral   @ref integral_param_t Pointer to integral parameters
//! \param detector     @ref detector_t Pointer to detector
//! \param x            @ref uint32_t X position of the patch to consider
//! \param y            @ref uint32_t Y position of the patch to consider
//! \param pfpNorm      @ref uint32_t pointer to normalize value used during cascade_CheckThreshold
//! \param ppixSum      @ref uint32_t pointer to the sum of pixel on x,y
//! \param psqPixSum    @ref uint32_t pointer to the sum of square pixel on x,y
//! \param pstdDev      @ref uint32_t pointer to the standard deviation on x,y
//!
//-----------------------------------------------------------------------------
void compute_stat(const integral_param_t * integral, const detector_t * detector, uint32_t x, uint32_t y, uint32_t * pfpNorm, uint32_t * ppixSum, uint32_t * psqPixSum, uint32_t * pstdDev)
{
  uint32_t pixSum;
  uint64_t sqPixSum;
  uint64_t StdSq;
  uint32_t Var;
  int32_t  fpNorm;
  uint32_t patchXEnd, patchYEnd;

  // Stat on integral image
  // 0           patchXend
  //
  //  a             b
  //    ---------------
  //    |             |
  //    |             |
  //    |             |
  //    |             |
  //  c |           d |
  //    ---------------
  // patchYend   patchXend
  // Sum of pixels in patch is : a - b - c + d
  //

  patchXEnd = detector->width-1;
  patchYEnd = detector->height-1;

  uint32_t a, b, c, d,xa,xb,xc,xd;

  int zeroExt = (integral->zeroExtend == true) ? 1 : 0;

  patchXEnd += zeroExt;
  patchYEnd += zeroExt;

  xa = x + (y * integral->currentWidth) - integral->xStart;
  xc = xa + (patchYEnd * integral->currentWidth);

  xb = xa+patchXEnd;
  xd = xc+patchXEnd;

  a = integral->integral_image[xa];
  b = integral->integral_image[xb];
  c = integral->integral_image[xc];
  d = integral->integral_image[xd];

  //    INFO(">>> %d, %d, %d, %d (%d, %d, %d, %d)", a, b, c, d, x, patchXEnd, yOffset, yEndOffset);

  pixSum  = a; //integral->integral_image[yOffset+x];
  pixSum += d; //integral->integral_image[yEndOffset+x+patchXEnd];
  pixSum -= b; //integral->integral_image[yOffset+x+patchXEnd];
  pixSum -= c; //integral->integral_image[yEndOffset+x];

  sqPixSum  = integral->sq_integral_image[xa];
  sqPixSum += integral->sq_integral_image[xd];
  sqPixSum -= integral->sq_integral_image[xb];
  sqPixSum -= integral->sq_integral_image[xc];


  StdSq = (int64_t)(sqPixSum * detector->area) - (int64_t) pixSum * pixSum;
  //    if (pixSum > 65525)
  //      fprintf(pFile, "tmp:%llu, StdSq:%llu,   ", tmp, StdSq );
  if(StdSq > 0){
    Var = sqrt64((const uint64_t *)& StdSq);
    fpNorm = (Var * detector->invArea) >> 8;
  }
  else {
    Var = detector->area;
    fpNorm = 1<<16;
  }

  INFO("[check_stat] @ %d, %d => avg = %d, stdDev = %d", x, y, pixSum/detector->area, Var);

  /* check cascade parameters */
  *ppixSum = pixSum;
  *psqPixSum = sqPixSum;
  *pstdDev = Var;
  *pfpNorm = fpNorm;
}

//-----------------------------------------------------------------------------
//! \brief  Check statistics of the current patch of the integral image
//!     to continue or not the processing.
//!
//! \param cascade       @ref cascade_t Pointer to current cascade
//! \param ppixSum       @ref uint32_t the sum of pixel on x,y to check
//! \param psqPixSum     @ref uint32_t the sum of square pixel on x,y to check
//! \param pstdDev       @ref uint32_t the standard deviation on x,y to check
//! \return bool         statistics which allow to continue investigation
//!
//-----------------------------------------------------------------------------
bool check_stat(cascade_t * cascade, uint32_t pixSum, uint32_t sqPixSum, uint32_t stdDev) {
  /* check cascade parameters */
  if ((pixSum > cascade->mean_min) &&
      (pixSum < cascade->mean_max) &&
      (sqPixSum > cascade->sw_min) &&
      (stdDev > cascade->var_min)) {
    return 1;
  }
  else {
    return 0;
  }
}

//-----------------------------------------------------------------------------
//! \brief  detect c_flavourless view.
//!
//! \param input            @ref IMG_image_t "Input image".
//! \param param            @ref detect_param_t "Algorithm parameters".
//! \param core_param     @ref core_param_t "Step Algorithm parameters".
//! \param pList        @ref RectList_t "Add each rect"
//!
//-----------------------------------------------------------------------------
void detect_core(const IMG_image_t * input, detect_param_t * param, core_param_t * core_param, RectList_t * pList)
{
  //    uint32_t stripeNumber, lastStripeSize;
  uint32_t height, width;
  uint32_t liiwWidth, liiwHeight;
  uint32_t xEnd, yEnd;
  uint32_t xStep, yStep;
  uint32_t x, y, c;
  uint8_t NbCascades;
  uint32_t Norm; // a mettre dans une structure ?????
  uint32_t pixSum, sqPixSum, stdDev;
  int yOff; // offset shift
  cascade_t * currentCascade;

  bool north, south, east, west;

  north = param->rotation & NORTH;
  south = param->rotation & SOUTH;
  east = param->rotation & EAST;
  west = param->rotation & WEST;

  //printf("rotation:%d, north:%d, south:%d, east:%d, west:%d\n",param->rotation, north, south, east, west);

#ifdef DEBUG_CASCADE
  uint32_t patchNumber;
#endif

  integral_param_t integral_param;

  NbCascades = core_param->detector->cascadeNb;

  //printf("height:%d, width:%d\n",input->height, input->width);
  
  // fprintf(pFile, "width_detector:%d, xinc:%d, inputW:%d\n",
  //    core_param->detector->width, core_param->xinc, input->width);
  
  // Compute some limits
  liiwWidth = core_param->detector->width + core_param->xinc - 1;
  liiwHeight = core_param->detector->height + core_param->yinc - 1;
  xEnd = input->width - liiwWidth + 1;
  xStep = core_param->xinc;
  yEnd = input->height - liiwHeight + 1;
  yStep = core_param->yinc;
  
  // printf("xEnd:%d, yEnd:%d\n", xEnd, yEnd);
  
  //printf("loop cascade: %d, %d\n", xStep, yStep);

  // patchWidth = core_param->detector->width - 1;
  //int32_t wWindow = liiwWidth + core_param->xinc/2 - 1;
  int32_t wWindow = core_param->detector->width - 1 + xStep - 1; // PatchSz + xinc - 1
  wWindow++; // faire le calcul exact

  // DEBUG("[detect_core] %d, %d, %d , %d",
  //  core_param->detector->width, core_param->xinc, input->width, liiwWidth);

  // Stripe width cannot be less than the local integral image window search width
  //    printf("iistripW:%d, iiW : %d\n", param->iiStripeWidth, liiwWidth);
  integral_param.stripeWidth = MAX(param->iiStripeWidth, liiwWidth);
  //    integral_param.stripePositions = integral_param.stripeWidth - liiwWidth + 1;

  // Stripe initialization
  integral_param.xStart = 0;
  integral_param.zeroExtend = true;

  // Shape of the computed integral image
  width = integral_param.stripeWidth;
  height = input->height;

  // Add an extra colum and an extra line for zeroExtentsion
  if (integral_param.zeroExtend) {
    width += 1 ;
    height += 1;
  }

  integral_param.integral_image = (uint32_t *) malloc((integral_param.stripeWidth+1)*height*sizeof(uint32_t));
  assert(integral_param.integral_image != NULL);
  integral_param.sq_integral_image = (uint64_t *) malloc((integral_param.stripeWidth+1)*height*sizeof(uint64_t));
  assert(integral_param.sq_integral_image != NULL);

  // memset(integral_param.integral_image, 0, (integral_param.stripeWidth+1)*height*sizeof(uint32_t)) ;
  // memset(integral_param.sq_integral_image, 0, (integral_param.stripeWidth+1)*height*sizeof(uint64_t)) ;

  // DEBUG("[detect_core] %d stripe(s) to process / %d position per stripe",
  //  stripeNumber, integral_param.stripePositions);

  /* calcul xStart and yStart in terms of step */
  int xStart = 0;
  int yStart = 0;

#if LIIW_SCAN_CENTERED

  int tmpxEnd = input->width - core_param->detector->width - xStep + 1;
  tmpxEnd = (tmpxEnd / xStep) * xStep + core_param->detector->width + xStep - 1;
  int tmpxStart = (input->width - tmpxEnd) / 2;
  tmpxEnd = (tmpxEnd - (core_param->detector->width + xStep - 1)) + tmpxStart;

  int tmpyEnd = input->height - core_param->detector->height - yStep + 1;
  tmpyEnd = (tmpyEnd / yStep) * yStep + core_param->detector->height + yStep - 1;
  int tmpyStart = (input->height - tmpyEnd) / 2;
  tmpyEnd = (tmpyEnd - (core_param->detector->height + yStep - 1)) + tmpyStart;

  xStart = tmpxStart;
  xEnd = tmpxEnd+1;
  yStart = tmpyStart;
  yEnd = tmpyEnd+1;

#endif

  integral_param.xStart = xStart;

  //fprintf(pFile, "\tXStart:%d, XEnd:%d, xstep:%d, \t\tYStart:%d, YEnd:%d, ystep:%d\n",
  //    xStart, xEnd, xStep, yStart, yEnd, yStep);

  //xEnd = xStart+xStep;

#if DEBUG_LEVEL > 1
  printf("xEnd = 0x%X, xStep = 0x%X\n",(unsigned int)xEnd, (unsigned int)xStep);
  printf("yEnd = 0x%X\n",(unsigned int)yEnd);
#endif

  // loop by raw
  for(x=xStart ; x<xEnd ; x+=xStep) {

    // compute integral image for the current stripe
    compute_integral(input, &integral_param);

#ifdef PULP  
    unsigned int failed = 0;

    if (PULP_pulp) {
      // need pointer to PulpDev struct
      PulpDev * pulp;
      pulp = PULP_pulp;

      // generate and populate task descriptor
      TaskDesc task;
      char app_name[15];
      strcpy(app_name,"face_detect");
        
      task.task_id    = 0;
      task.name       = &app_name[0];
      task.n_clusters = -1;
      task.n_data     = 7;
    
      DataDesc data_desc[7];
      data_desc[0].ptr  = core_param;
      data_desc[0].size = sizeof(core_param_t);
      data_desc[0].type = 1;

      data_desc[1].ptr  = pList;
      data_desc[1].size = sizeof(RectList_t);
      data_desc[1].type = 1;

      data_desc[2].ptr  = &integral_param;
      data_desc[2].size = sizeof(integral_param_t);
      data_desc[2].type = 1;

      data_desc[3].ptr  = &width;
      data_desc[3].size = sizeof(uint32_t);
      data_desc[3].type = 1;
    
      data_desc[4].ptr  = &height;
      data_desc[4].size = sizeof(uint32_t);
      data_desc[4].type = 1;

      data_desc[5].ptr  = &x;
      data_desc[5].size = sizeof(uint32_t);
      data_desc[5].type = 1;

      data_desc[6].ptr  = &(param->rotation);
      data_desc[6].size = sizeof(int);
      data_desc[6].type = 1;

      task.data_desc = &data_desc[0];

#if MEM_SHARING == 1
      /*
       * core_param
       */ 
      // allocate, copy & change pointers
      core_param_t * core_param_cv; // host accesses the structure in contiguous L3 through virtual memory
      unsigned core_param_cp; // PULP accesses directly using the physical address   
      core_param_cv = (core_param_t *)pulp_l3_malloc(pulp, sizeof(core_param_t), &(core_param_cp));
      memcpy((void *)core_param_cv, (void *)core_param, sizeof(core_param_t));
      
      detector_t * detector_cv;
      unsigned  detector_cp;
      detector_cv = (detector_t *)pulp_l3_malloc(pulp, sizeof(detector_t), &(detector_cp));
      memcpy((void *)detector_cv, (void *)core_param->detector, sizeof(detector_t));
      
      core_param_cv->detector = (detector_t *)detector_cp;

      cascade_t * cascade_cv;
      unsigned cascade_cp;
      cascade_cv = (cascade_t *)pulp_l3_malloc(pulp, NbCascades*sizeof(cascade_t), &(cascade_cp));
      memcpy((void *)cascade_cv, (void *)core_param->detector->cascade, NbCascades*sizeof(cascade_t));

      detector_cv->cascade = (cascade_t *)cascade_cp;

      stage_t ** c_stages_cv = (stage_t **)malloc(NbCascades*sizeof(stage_t *));
      unsigned * c_stages_cp = (unsigned *)malloc(NbCascades*sizeof(unsigned *));
      if (!c_stages_cv || !c_stages_cp)
        printf("Malloc failed for c_stages_cv/c_stages_cp.\n"); 

      weak_t ** c_weaks_cv = (weak_t **)malloc(NbCascades*sizeof(weak_t *));
      unsigned * c_weaks_cp = (unsigned *)malloc(NbCascades*sizeof(unsigned *));
      if (!c_weaks_cv || !c_weaks_cp)
        printf("Malloc failed for c_weaks_cv/c_weaks_cp.\n"); 

      int i;
      int NbStages;
      int NbWeaks;
      for (i=0; i<NbCascades; i++) {
        NbStages = (int)(core_param->detector->cascade[i].nb_stage);
        NbWeaks = (int)(core_param->detector->cascade[i].stages[NbStages-1].nWeak)
          + (int)(core_param->detector->cascade[i].stages[NbStages-1].WeaksOffset);

        // allocate, copy and change pointers
        c_stages_cv[i] = (stage_t *)pulp_l3_malloc(pulp, NbStages*sizeof(stage_t), &(c_stages_cp[i]));  
        memcpy((void *)c_stages_cv[i], (void *)(core_param->detector->cascade[i].stages), NbStages*sizeof(stage_t));
        
        cascade_cv[i].stages = (stage_t *)c_stages_cp[i];

        c_weaks_cv[i] = (weak_t *)pulp_l3_malloc(pulp, NbWeaks*sizeof(weak_t), &(c_weaks_cp[i]));  
        memcpy((void *)c_weaks_cv[i], (void *)(core_param->detector->cascade[i].weaks), NbWeaks*sizeof(weak_t));
        
        cascade_cv[i].weaks = (weak_t *)c_weaks_cp[i];

        // print size of cascade
        //printf("Size Cascade %i = %0.2f KiB\n",i,((float)(NbStages*sizeof(stage_t)+NbWeaks*sizeof(weak_t)))/1024);
      }
      
      uint8_t * scanOrder_cv;
      unsigned  scanOrder_cp;
      if (core_param->scanOrder == ScanOrder1x1) {
        scanOrder_cv = (uint8_t *)pulp_l3_malloc(pulp, 1*2*sizeof(uint8_t), &(scanOrder_cp));
        memcpy((void *)scanOrder_cv, (void *)(core_param->scanOrder), 1*2*sizeof(uint8_t));
      }
      else {
        scanOrder_cv = (uint8_t *)pulp_l3_malloc(pulp, 25*2*sizeof(uint8_t), &(scanOrder_cp));
        memcpy((void *)scanOrder_cv, (void *)(core_param->scanOrder), 25*2*sizeof(uint8_t));
      }
      core_param_cv->scanOrder = (uint8_t *)scanOrder_cp;
 
      /*
       * integral_param
       */ 
      // allocate, copy & change pointers
      unsigned size = (integral_param.stripeWidth+1) * height;
      integral_param_t * integral_param_cv;
      unsigned integral_param_cp;
      integral_param_cv = (integral_param_t *)pulp_l3_malloc(pulp, sizeof(integral_param_t), &(integral_param_cp));
      memcpy((void *)integral_param_cv, (void *)&integral_param, sizeof(integral_param_t));
      
      // integral_param->integral_image
      uint32_t * integral_image_cv;
      unsigned integral_image_cp;
      integral_image_cv = (uint32_t *)pulp_l3_malloc(pulp, size*sizeof(uint32_t), &(integral_image_cp));
      memcpy((void *)integral_image_cv, (void *)integral_param.integral_image, size*sizeof(uint32_t));

      integral_param_cv->integral_image = (uint32_t *)integral_image_cp;

      // integral_param->sq_integral_image
      uint64_t * sq_integral_image_cv;
      unsigned sq_integral_image_cp;
      sq_integral_image_cv = (uint64_t *)pulp_l3_malloc(pulp, size*sizeof(uint64_t), &(sq_integral_image_cp));
      memcpy((void *)sq_integral_image_cv, (void *)integral_param.sq_integral_image, size*sizeof(uint64_t));
      
      integral_param_cv->sq_integral_image = (uint64_t *)sq_integral_image_cp;

      // change pointers in task struct
      data_desc[0].ptr  = (void *)core_param_cp;
      data_desc[2].ptr  = (void *)integral_param_cp;

      //printf("__LINE__ = %d\n",__LINE__);
#endif

      // start offload
      pulp_write32(pulp->mailbox.v_addr, MAILBOX_WRDATA_OFFSET_B, 'b', PULP_START);
    
      // pass task descriptor
      pulp_offload_out(pulp, &task);

      // debug prints
      //#define PRINT
#ifdef PRINT
      printf("----\n");

      printf("xStart            @ 0x%X = 0x%X\n",(unsigned int)&(integral_param.xStart)           , (unsigned int)(integral_param.xStart));
      printf("zeroExtend        @ 0x%X = 0x%X\n",(unsigned int)&(integral_param.zeroExtend)       , (unsigned int)(integral_param.zeroExtend));
      printf("stripeWidth       @ 0x%X = 0x%X\n",(unsigned int)&(integral_param.stripeWidth)      , (unsigned int)(integral_param.stripeWidth));
      printf("currentWidth      @ 0x%X = 0x%X\n",(unsigned int)&(integral_param.currentWidth)     , (unsigned int)(integral_param.currentWidth));
 
      printf("integral_image    @ 0x%X = 0x%X\n",(unsigned int)&(integral_param.integral_image)   , (unsigned int)(integral_param.integral_image));
      printf("sq_integral_image @ 0x%X = 0x%X\n",(unsigned int)&(integral_param.sq_integral_image), (unsigned int)(integral_param.sq_integral_image));

#if (MEM_SHARING == 1)
      printf("xStart            @ 0x%X = 0x%X\n",(unsigned int)&(integral_param_cv->xStart)           , (unsigned int)(integral_param_cv->xStart));
      printf("zeroExtend        @ 0x%X = 0x%X\n",(unsigned int)&(integral_param_cv->zeroExtend)       , (unsigned int)(integral_param_cv->zeroExtend));
      printf("stripeWidth       @ 0x%X = 0x%X\n",(unsigned int)&(integral_param_cv->stripeWidth)      , (unsigned int)(integral_param_cv->stripeWidth));
      printf("currentWidth      @ 0x%X = 0x%X\n",(unsigned int)&(integral_param_cv->currentWidth)     , (unsigned int)(integral_param_cv->currentWidth));
 
      printf("integral_image    @ 0x%X = 0x%X\n",(unsigned int)&(integral_param_cv->integral_image)   , (unsigned int)(integral_param_cv->integral_image));
      printf("sq_integral_image @ 0x%X = 0x%X\n",(unsigned int)&(integral_param_cv->sq_integral_image), (unsigned int)(integral_param_cv->sq_integral_image));
#endif
     
      printf("----\n");
      
      //printf("core_param @ %#x points to %p\n", &core_param, core_param);
      //printf("detector   @ %#x points to %p\n", &(core_param->detector), core_param->detector);
      //printf("cascadeNb  @ %#x points to %p\n", &(core_param->detector->cascadeNb),
      //      core_param->detector->cascadeNb);
      
      //printf("detector @ %p\n", core_param->detector);
      //printf("detector->cascadeNb @ %p\n", &(core_param->detector->cascadeNb));
      //printf("detector->cascadeNb = %x\n", core_param->detector->cascadeNb);
      //printf("detector->height @ %p\n", &(core_param->detector->height));
      //printf("detector->height = %x\n", core_param->detector->height);
#endif // PRINT

      // wait for offload end
      unsigned int ret;
      pulp_mailbox_read(pulp, &ret, 1);
      if (ret != PULP_DONE) {
        printf("ERROR: PULP execution failed.\n");
        //return;
        failed = 1;
        pulp_rab_free(pulp, (unsigned char)0xFF);
      }

      usleep(100000);

      // free RAB slices
      unsigned char date_cur = (unsigned char)(task.task_id + 4);
      pulp_rab_free(pulp, date_cur);
    
#if (MEM_SHARING == 1)
      // free
      pulp_l3_free(pulp, (unsigned)sq_integral_image_cv, sq_integral_image_cp);
      pulp_l3_free(pulp, (unsigned)integral_image_cv, integral_image_cp);
      pulp_l3_free(pulp, (unsigned)integral_param_cv, integral_param_cp);
      pulp_l3_free(pulp, (unsigned)scanOrder_cv, scanOrder_cp);
      for (i=0; i<NbCascades; i++) {
        pulp_l3_free(pulp, (unsigned)c_weaks_cv[i], c_weaks_cp[i]);
        pulp_l3_free(pulp, (unsigned)c_stages_cv[i], c_stages_cp[i]);
      }
      free(c_weaks_cp);
      free(c_weaks_cv);
      free(c_stages_cv);
      free(c_stages_cp);
      pulp_l3_free(pulp, (unsigned)cascade_cv, cascade_cp);
      pulp_l3_free(pulp, (unsigned)detector_cv, detector_cp);
      pulp_l3_free(pulp, (unsigned)core_param_cv, core_param_cp);
#endif
    }

    if (PULP_pulp) {
      // loop by line
      for(y=yStart ; y<yEnd ; y+=yStep) {

#if DEBUG_LEVEL > 1
        printf("y = %d\n", (int)y);
#endif       

        int32_t X, Y;

        // to be compatible with RAID algo version
        // but !!!!! TO BE CLARIFY !!!!!!!!!!!!!
        X = x+(xStep-1)/2;
        Y = y+(yStep-1)/2;

        // Execute the cascade on patch at x,y
        yOff = y; // offset shift

        // compute stat for the current patch
        compute_stat(&integral_param, core_param->detector, X, Y, &Norm, &pixSum, &sqPixSum, &stdDev);

        // at the first face found no additional cascade or rotation should be executed
        bool stop;

        for (c = 0, stop = false; !stop && (c < NbCascades); c++){

#if DEBUG_LEVEL > 3
          printf("x = %d, y = %d, c = %d\n", (int)x, (int)y, (int)c);
#endif

          // get current cascade
          currentCascade = &(core_param->detector->cascade[c]);

          // for each cascade try rotations
          if ( check_stat(currentCascade, pixSum, sqPixSum, stdDev) || core_param->disableStats ) {
            if(north){
              stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep, 
                                          core_param->scanOrder, currentCascade, 
                                          core_param->detector->width - 1,
                                          core_param->detector->height - 1,
                                          integral_param.currentWidth, 
                                          yOff, Norm, core_param->ScalFactor, pList, c, NORTH);
            }
            if(!stop && west){
              stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep,
                                          core_param->scanOrder, currentCascade,
                                          core_param->detector->width - 1,
                                          core_param->detector->height - 1, 
                                          integral_param.currentWidth, 
                                          yOff, Norm, core_param->ScalFactor, pList, c, WEST);
            }
            if(!stop && east){
              stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep,
                                          core_param->scanOrder, currentCascade, 
                                          core_param->detector->width - 1, 
                                          core_param->detector->height - 1, 
                                          integral_param.currentWidth,
                                          yOff, Norm, core_param->ScalFactor, pList, c, EAST);
            }
            if(!stop && south){
              stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep,
                                          core_param->scanOrder, currentCascade,
                                          core_param->detector->width - 1,
                                          core_param->detector->height - 1,
                                          integral_param.currentWidth, yOff, Norm,
                                          core_param->ScalFactor, pList, c, SOUTH);
            }
          } // if check_stat
        } // for NbCascades
      } // for yEnd
    } // if PULP_pulp

    if (failed)
      PULP_pulp = 0;

#else // PULP
    // loop by line
    for(y=yStart ; y<yEnd ; y+=yStep) {

      int32_t X, Y;

      // to be compatible with RAID algo version
      // but !!!!! TO BE CLARIFY !!!!!!!!!!!!!
      X = x+(xStep-1)/2;
      Y = y+(yStep-1)/2;

      // Execute the cascade on patch at x,y
      yOff = y; // offset shift

      // compute stat for the current patch
      compute_stat(&integral_param, core_param->detector, X, Y, &Norm, &pixSum, &sqPixSum, &stdDev);

      // at the first face found no additional cascade or rotation should be executed
      bool stop;

      for (c = 0, stop = false; !stop && (c < NbCascades); c++){

        // get current cascade
        currentCascade = &(core_param->detector->cascade[c]);

        // for each cascade try rotations
        if ( check_stat(currentCascade, pixSum, sqPixSum, stdDev) || core_param->disableStats ) {
          if(north){
            stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep, 
                                        core_param->scanOrder, currentCascade, 
                                        core_param->detector->width - 1,
                                        core_param->detector->height - 1,
                                        integral_param.currentWidth, 
                                        yOff, Norm, core_param->ScalFactor, pList, c, NORTH);
          }
          if(!stop && west){
            stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep,
                                        core_param->scanOrder, currentCascade,
                                        core_param->detector->width - 1,
                                        core_param->detector->height - 1, 
                                        integral_param.currentWidth, 
                                        yOff, Norm, core_param->ScalFactor, pList, c, WEST);
          }
          if(!stop && east){
            stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep,
                                        core_param->scanOrder, currentCascade, 
                                        core_param->detector->width - 1, 
                                        core_param->detector->height - 1, 
                                        integral_param.currentWidth,
                                        yOff, Norm, core_param->ScalFactor, pList, c, EAST);
          }
          if(!stop && south){
            stop = execute_cascade_liiw(integral_param.integral_image, x, y, xStep, yStep,
                                        core_param->scanOrder, currentCascade,
                                        core_param->detector->width - 1,
                                        core_param->detector->height - 1,
                                        integral_param.currentWidth, yOff, Norm,
                                        core_param->ScalFactor, pList, c, SOUTH);
          }
        } // if check_stat
      } // for NbCascades
    } // for yEnd
#endif // PULP

    // increment start position on the stripe
    integral_param.xStart += xStep;
  } // for xEnd

#ifdef DEBUG_CASCADE
  DEBUG("[detect_core] execute_cascade_liiw on %d patche(s) %dx%d", patchNumber,  xStep, yStep);
#endif

  // Free allocated memory for integral image computation
  //     free(integral_param.integral_image);
  //     free(integral_param.sq_integral_image);
}

