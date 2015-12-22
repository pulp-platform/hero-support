/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: integral.c.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: integral.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include "raid.h"
#include <stdlib.h>
#include "integral.h"

//-----------------------------------------------------------------------------
//! \brief  Compute the integral image and the integral image of the square
//!
//! \param input		@ref IMG_image_t Pointer to the input picture
//! \param param    	@ref integral_param_t Pointer to integral parameters
//!
//-----------------------------------------------------------------------------
void compute_integral(const IMG_image_t *input, integral_param_t *param)
{
    uint32_t * ii_ptr;
    uint64_t * sq_ptr;
    uint32_t width;
    IMG_data_t pixel;
    uint32_t sum;
    uint64_t sqsum;
    uint32_t xStart, xEnd;
    uint32_t x, y;
    uint32_t pixelIndex;

    xStart = param->xStart ;
    xEnd = MIN(param->xStart + param->stripeWidth, (uint32_t) input->width);

    width = xEnd - xStart;
    if (param->zeroExtend) {
        width = width + 1;
    }

    param->currentWidth = width;

    /* first line */
    ii_ptr = param->integral_image;
    sq_ptr = param->sq_integral_image;
    if (param->zeroExtend) {
        // Skip first line which is forced to 0
        for(x=0; x < width; x++){
        	*ii_ptr = 0;
        	*sq_ptr = 0;
    		ii_ptr++;
    		sq_ptr++;
        }
    }

    /* next lines */
    for (y=0 ; y < (uint32_t) input->height ; y++) {
        if (param->zeroExtend) {
            // Skip first column which is forced to 0
        	*ii_ptr = 0;
        	*sq_ptr = 0;
            ii_ptr++;
            sq_ptr++;
        }
        sum = 0;
        sqsum = 0LL;
        for (x = xStart ; x < xEnd ; x++, pixelIndex++) {
            pixel = *IMG_PIXEL(*input, x, y);
            sum += pixel;
            sqsum += pixel*pixel;
            *ii_ptr = sum + *(ii_ptr-width);
            *sq_ptr = sqsum + *(sq_ptr-width);

            ii_ptr++;
            sq_ptr++;
        }
    }
}
