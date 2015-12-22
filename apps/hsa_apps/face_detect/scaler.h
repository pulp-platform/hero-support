/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: scaler.h.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: scaler.h.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#ifndef __SCALER_H__
#define __SCALER_H__

typedef struct {
    int x ;
    int y ;
    int width ;
    int height ;
} woi_t ;

uint32_t scale(const IMG_image_t *source, IMG_image_t *dest, woi_t *woi) ;

#endif // __SCALER_H__
