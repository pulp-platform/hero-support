/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: scaler.c.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: scaler.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include "raid.h"
#include "scaler.h"


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

//-----------------------------------------------------------------------------
//! \brief 		Bilinear scaling. Faster than using a more
//! 			refined scaler that represents the hardware.
//!
//! Image size for scaling and window of interest
//! should be specified on the destination image
//! structure.
//!
//! \param source        Pointer to source image
//! \param dest          Pointer to scaled image
//! \param woi           Window of interest
//! \return              Scaling factor in fixed point 16
 //-----------------------------------------------------------------------------
uint32_t scale(const IMG_image_t *source, IMG_image_t *dest, woi_t *woi) {
    // Declaring auxiliary variables for scaling operation
    int   x,   y;
    int  xv,  yv;
    int  xi,  yi;
    int  dx,  dy;
    int   a,   b;
    int tmp1, tmp2;

    int lineWidth;
    int inH, inW;
    int inX0, inY0;
    int outH, outW;

    int step;

    // Buffer management pointers
    IMG_data_t *in, *inend;
    IMG_data_t *out;

    lineWidth = source->width;
    in = source->data;
    inend = in + lineWidth * source->height;

    // Manage window of interest
    inX0 = woi->x ;
    inY0 = woi->y ;
    inW  = woi->width ;
    inH  = woi->height ;

//    out = dest->data; // redondant !!!!
    outW = dest->width;
    outH = dest->height;
    out = dest->data;

    // FIXME: Reset image content (useless ???)
    memset(out, 0, (outW*outH)*sizeof(IMG_data_t));


    // Peek min of the two steps not to overflow
    // in any direction the input array while resampling
    step = MIN( (inH<<8)/(outH), (inW<<8)/(outW) );

    yv = inY0 << 8 ;
    for (y = 0 ; y < outH ; y++) {
        yi = yv >> 8;
        dy = yv & 0xFF;

        xv = inX0 << 8;
        for (x = 0 ; x < (outW) ; x++) {
            xi = xv >> 8;
            dx = xv & 0xFF;

            // Pixels interpolation:
            // a b
            //  X
            // c d
            // First interpolate 'a' and 'b'
            in = source->data + xi + yi * lineWidth ;
            a = *in;
            if (in < inend - 1) {
                b = *(in+1);
            }
            else {
                b = a ;
            }
            tmp1 = (a << 8) + (b - a) * dx;

            // Then interpolate 'c' and 'd'.
            // Finally compute result of 'a' and 'b'
            // interpolated with result of 'c' and 'd'.
            in += lineWidth;
            if (in < inend - 1) {
                a = *in;
                b = *(in + 1);
                tmp2 = (a << 8) + (b - a) * dx;
                tmp1 += ((tmp2 - tmp1) * dy) >> 8;
            }
            *out++ = (IMG_data_t)(tmp1 >> 8);
            xv += step;
        }
        yv += step;
    }

    // Returns scale factor as 16 bit fixed point value
    return (step << 8);
}

