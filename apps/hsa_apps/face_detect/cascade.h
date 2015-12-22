/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: cascade.h.rca 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: cascade.h.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:52 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#ifndef __CASCADE_H__
#define __CASCADE_H__

#include "list.h"
#include "detect.h" // for rotation_e
//#include "detect_core.h" // for core_param_t

typedef enum {
    FACE_1_2_9,
    DETECTOR_TYPE_NUMBER
} detector_type_e;


typedef enum {
	NO_SYMMETRY,
	V_SYMMETRY,
    H_SYMMETRY // never used ??????
} symmetry_e;

typedef struct {
    int16_t nWeak;
    int16_t WeaksOffset;
    int32_t StrongThr;
    int32_t AlphaSum;
} stage_t;

typedef struct {
    int16_t type;
    int16_t alpha;
    int8_t  r, c, jump_r, jump_c;
    int32_t thr1, thr2;
} weak_t;

typedef struct {
    unsigned int mean_min;
    unsigned int mean_max;
    unsigned int sw_min;
    unsigned int var_min;
    symmetry_e symmetry;
    char nb_stage;
    stage_t *stages;
    weak_t  *weaks;
} cascade_t;

typedef struct {
    uint8_t width;
    uint8_t height;
    uint16_t area;
    uint16_t invArea;
    uint8_t cascadeNb;
    cascade_t * cascade;
} detector_t;

typedef struct {
	uint16_t level; // last tested level of the cascade
	int32_t value;  // last result value of the cascade
	int16_t result; // last result of the cascade (true/false)
} result_t;


extern const uint8_t ScanOrder1x1[];
extern const uint8_t ScanOrder5x5[];


const detector_t * get_detector(uint8_t cascade) ;


bool execute_cascade_liiw(const uint32_t * integral_image,
						  uint32_t x, uint32_t y, uint32_t xinc, uint32_t yinc, const uint8_t * scanOrder,
						  cascade_t * currentCascade, int32_t patchWidth, int32_t patchHeigth, int32_t wWindow,
						  int32_t yOff, int32_t Norm, int32_t ScalFactor, RectList_t * pList, int8_t cascade, int rotation);

void execute_cascade(const uint32_t * integral_image, uint32_t x, uint32_t y, cascade_t * currentCascade, int32_t wWindow, int32_t yOff, int32_t patchWidth, int32_t patchHeight, int32_t Norm, result_t *pResult, int rotation);

int Cascade_GetFeatureValue(weak_t * pWeak, const uint32_t * integral_image, int32_t wWindow, int32_t symmetry, int32_t patchWidth, int32_t patchHeight, int x, int y, int32_t yOff, int rotation);

int Cascade_CheckThreshold(weak_t * pWeak, int32_t Value, int32_t Norm);


#endif // __CASCADE_H__

