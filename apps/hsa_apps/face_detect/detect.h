/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: detect.h.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: detect.h.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#ifndef __DETECT_H__
#define __DETECT_H__
#include "list.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
		NORTH = 1,
		SOUTH = 2,
	    EAST = 4,
	    WEST = 8
} rotation_e;


/**
 * Algorithm parameters.
 */
typedef struct {
    uint8_t cascade ;
    uint32_t iiStripeWidth ;
    double scalingStep ;
    int16_t validScore;
    int16_t keepScore;
    char xStep ;
    char yStep ;
    int rotation;
    //object_list_t * list ;
    RectList_t list;
} detect_param_t;


void detect(const IMG_image_t *input, detect_param_t *param);

#ifdef __cplusplus
}
#endif

#endif // __DETECT_H__









