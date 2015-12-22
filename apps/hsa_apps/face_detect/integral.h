/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: integral.h.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: integral.h.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#ifndef  __INTEGRAL_H__
#define  __INTEGRAL_H__
#include <raid.h>

typedef struct {
    // Extend integral stripe by a 0 line and an 0 column
    bool zeroExtend;

    // X start for the strip mode management
    uint32_t xStart;

    // To ensure no overflow of the integral image values
    // it may be necessary to compute it by stripes
    uint32_t stripeWidth;
//    uint32_t stripePositions;
    uint32_t currentWidth;

    // Pointers to integral image and square integral image
    uint32_t * integral_image;
    uint64_t * sq_integral_image;

} integral_param_t;

void compute_integral(const IMG_image_t * input, integral_param_t *param);

#endif // __INTEGRAL_H__

