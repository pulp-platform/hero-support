/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: detect_core.h.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: detect_core.h.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#ifndef __DETECT_CORE_H__
#define __DETECT_CORE_H__
#include "cascade.h"

typedef struct {
    uint8_t xinc;
    uint8_t yinc;
    bool disableStats;
    int32_t ScalFactor;
    const detector_t * detector;
    const uint8_t * scanOrder;
} core_param_t;

void detect_core(const IMG_image_t * input, detect_param_t * param, core_param_t * core_param, RectList_t * pList);
//void detect_core(const IMG_image_t * input, detect_param_t * param, core_param_t * core_param);


#endif // __DETECT_CORE_H__

