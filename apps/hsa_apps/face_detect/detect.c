/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: detect.c.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: detect.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include "raid.h"
#include "scaler.h"
#include "cascade.h"
#include "detect.h"
#include "detect_core.h"

//-----------------------------------------------------------------------------
//! \mainpage
//! \author Imaging Division\n
//!			ISP Architecture and Image Analytics\n
//!			Odile Rey\n
//! \version v1.0
//! \date October 10 2012
//!
//!	\brief	The code is the reference code for the faces detection algorithm.\n
//! 		It will be the start point for other application based on faces
//!			detection.\n
//!
//-----------------------------------------------------------------------------

//#define DEBUG_SCALING


//-----------------------------------------------------------------------------
//! \brief 	Update core parameters (xinc, yinc dans scanorder
//!			according to the previous value and the size of the windows
//!
//! \param param		 @ref detect_param_t Pointer to detector parameters
//! \param core_param    @ref core_param_t Pointer to core parameters
//! \param min_length    smallest size of the side of the windows
//-----------------------------------------------------------------------------
void update_core_param(detect_param_t *param, core_param_t * core_param, uint32_t min_length) {

  uint32_t patchSize = core_param->detector->width;
  DEBUG("[update_core_param] patch size %dx%d", patchSize, patchSize);
  // xStep and yStep defined by main.py
  // -1: Auto, according to the patch size
  if ((param->xStep == (char)-1) &&  (param->yStep == (char)-1)) {
    // Automatic setup in increment step
    if (min_length > (patchSize + (3*patchSize+2)/4)) {//((11*patchSize+2)/4)) {
      core_param->xinc = 5;
      core_param->yinc = 5;
      core_param->scanOrder = ScanOrder5x5;
    } else {
      core_param->xinc = 1 ;
      core_param->yinc = 1;
      core_param->scanOrder = ScanOrder1x1;
    }
  }
  else {
    // Forced increment
    switch (param->xStep) {
    case 1:
      core_param->xinc = 1 ;
      core_param->yinc = 1;
      core_param->scanOrder = ScanOrder1x1;
      break;
    case 5:
      if (min_length > patchSize + 4) {
	core_param->xinc = 5;
	core_param->yinc = 5;
	core_param->scanOrder = ScanOrder5x5;
      }
      else {
	core_param->xinc = 1 ;
	core_param->yinc = 1;
	core_param->scanOrder = ScanOrder1x1;
      }
      break;
    default:
      CHECK(false, "Step increment %d / %d is not supported", param->xStep,param->yStep);
    }
  }
}


//-----------------------------------------------------------------------------
//! \brief 	Function to detect faces in input picture
//! 		with given detector parameters
//! 		List of detected face is updated in the detector parameters
//!
//! \param input		@ref IMG_image_t Pointer to the input picture
//! \param param    	@ref detect_param_t	Pointer to detector parameters
//!
//-----------------------------------------------------------------------------
void detect(const IMG_image_t *input, detect_param_t *param)
{
#ifdef DEBUG_SCALING
  char tmpStr[1024];
#endif

  bool inverScale = 1;
  unsigned int i;
  IMG_image_t scaled_output;
  woi_t woi;
  unsigned int stepNumber;
  unsigned int height, width;
  double scalingFactor, currentScalingFactor;
  core_param_t core_param;
  RectList_t RectList;

  RectListInit(&RectList);
  RectListFree(&param->list); //RectListInit(&param->list);

  core_param.detector = get_detector(param->cascade);

  // Compute scaling step number to reach the smallest image
  // fitting the detector size
  scalingFactor = param->scalingStep;

  height = input->height;
  width = input->width;
  stepNumber = 0;

  // printf("height = %d\n",height);
  // printf("width = %d\n",width);
  // 
  // printf("core_param.detector->height = %d\n",core_param.detector->height);
  // printf("core_param.detector->width = %d\n",core_param.detector->width);

  // check image size
  if (height < core_param.detector->height || width < core_param.detector->width) {
    return;
  }

  float fheight = (float)height;
  float fwidth = (float) width;

  currentScalingFactor = 1;

  // calculate the number of scaling factors must be proceeded
  while ( (height >= core_param.detector->height)
         && (width >= core_param.detector->width) ) {
    stepNumber += 1;

    fheight = fheight * scalingFactor;
    fwidth = fwidth * scalingFactor;

    currentScalingFactor *= scalingFactor;

    height = (unsigned int) (fheight);
    width = (unsigned int) (fwidth);
  }
  DEBUG("[detect] Number of scaling steps: %d", stepNumber) ;

  // Multi-scale loop
  // For each scaling step defined by scalingFactor,
  // scale the image and run the detection on the defined step
  if(inverScale){
    // start by the biggest face
    currentScalingFactor /= scalingFactor;
  }
  else{
    // start by the biggest scaling factor
    currentScalingFactor = 1;
  }

  // Setup the window of interest
  woi.x = 0;
  woi.y = 0;
  woi.width = input->width;
  woi.height = input->height;

  // Allocate the scaled image
  IMG_Create(&scaled_output, input->width, input->height, IMG_TYPE_GREY, 8);

  // for debugging
  printf("stepNumber = %d\n",(int)stepNumber);
  //stepNumber = 1;

  i=0;
  do{
    // Compute output size
    scaled_output.width = (int) (input->width * currentScalingFactor);
    scaled_output.height =  (int) (input->height * currentScalingFactor);

    DEBUG("[detect] Run detection on scaling step %d with ratio: %f, %dx%d", i, currentScalingFactor, scaled_output.width, scaled_output.height);

    core_param.ScalFactor = scale(input, &scaled_output, &woi);

    update_core_param(param, &core_param, MIN(scaled_output.width, scaled_output.height));
    
    //printf("Line %d of file \"%s\".\n",__LINE__, __FILE__);

    detect_core(&scaled_output, param, &core_param, &RectList);

    // Update scaling factor for next iteration
    if(inverScale){
      currentScalingFactor /= scalingFactor;
    } else {
      currentScalingFactor *= scalingFactor ;
    }

#ifdef DEBUG_SCALING
    sprintf(tmpStr, "scaled_out_%dx%d.pgm",  scaled_output.width, scaled_output.height);
    IMG_Save(&scaled_output, tmpStr, IMG_FORMAT_DEFAULT);
#endif

    i++;

  } while (i < stepNumber);

  ValidateResultList(&RectList, &param->list, param->validScore, param->keepScore);

  // Free the temporary faces list
  RectListFree(&RectList);
  // Free the scaled image
  IMG_Free(&scaled_output);
}

