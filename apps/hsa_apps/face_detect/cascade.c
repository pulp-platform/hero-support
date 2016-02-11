/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: cascade.c.rca 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: cascade.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include "raid.h"
#include "cascade.h"

//#define FACE_1_2_9


#ifdef FACE_1_2_9
#include "cascade_face_1_2_9.dat"
#else
#include "cascade_face_4_0_4mixed.dat"
#endif

#define FIND_HIT_ACCURATE_POSITION

// Scan order for 1x1 patches
//    | 0 |
// ---+---+
//  0 | 0 |
// ---+---+
const uint8_t ScanOrder1x1[1*2] = {
    0, 0  // x,y 0 @0 or 0,0
};


// Scan order for 5x5 patches
//    | 0   1   2   3   4 |
// ---+-------------------+
//  0 | 8   6  10   4  12 |
//  1 |13  14   0  15  16 |
//  2 | 3  17   7  19  18 |
//  3 |20   1  21   2  22 |
//  4 | 9  23   5  11  24 |
// ---+-------------------+
const uint8_t ScanOrder5x5[25*2] = {
    2, 1, // x,y 0 @7   or 0,-1
    1, 3, // x,y 1 @16  or -1,1
    3, 3, // x,y 2 @18  or 1,1
    0, 2, // x,y 3 @10  or -2,0
    3, 0, // x,y 4 @3   or 1,-2
    2, 4, // x,y 5 @22  or 0,2
    1, 0, // x,y 6 @1   or -1,-2
    2, 2, // x,y 7 @12  or 0,0
    0, 0, // x,y 8 @0   or -2,-2
    0, 4, // x,y 9 @20  or -2,2
    2, 0, // x,y 10 @2  or 0,-2
    3, 4, // x,y 11 @23 or 1,2
    4, 0, // x,y 12 @4  or 2,-2
    0, 1, // x,y 13 @5  or -2,-1
    1, 1, // x,y 14 @6  or -1,-1
    3, 1, // x,y 15 @8  or 1,-1
    4, 1, // x,y 16 @9  or 2,-1
    1, 2, // x,y 17 @11 or -1,0
    4, 2, // x,y 18 @14 or 2,0
    3, 2, // x,y 19 @13 or 1,0
    0, 3, // x,y 20 @15 or -2,1
    2, 3, // x,y 21 @17 or 0,1
    4, 3, // x,y 22 @19 or 2,1
    1, 4, // x,y 23 @21 or -1,2
    4, 4, // x,y 24 @24 or 2,2
};


static const detector_t detector_rom[DETECTOR_TYPE_NUMBER] = {
#ifdef FACE_1_2_9
    {FACE_1_2_9_DETECTOR_PATCH_SIZEX, FACE_1_2_9_DETECTOR_PATCH_SIZEY, (FACE_1_2_9_DETECTOR_PATCH_SIZEX*FACE_1_2_9_DETECTOR_PATCH_SIZEY), ((1 << 16)/(FACE_1_2_9_DETECTOR_PATCH_SIZEX*FACE_1_2_9_DETECTOR_PATCH_SIZEY)), FACE_1_2_9_CASCADE_NUMBER, face_1_2_9_cascades}
#else
    {FACE_4_0_4MIXED_DETECTOR_PATCH_SIZEX, FACE_4_0_4MIXED_DETECTOR_PATCH_SIZEY, (FACE_4_0_4MIXED_DETECTOR_PATCH_SIZEX*FACE_4_0_4MIXED_DETECTOR_PATCH_SIZEY), ((1 << 16)/(FACE_4_0_4MIXED_DETECTOR_PATCH_SIZEX*FACE_4_0_4MIXED_DETECTOR_PATCH_SIZEY)), FACE_4_0_4MIXED_CASCADE_NUMBER, face_4_0_4MIXED_cascades}
#endif
};

//-----------------------------------------------------------------------------
//! \brief Get the detector corresponding to the selected cascade
//!
//! Return the detector information corresponding to the selected cascade to
//! pass information in the core parameters
//!
//! \param cascade Number of the cascade to use
//! \return Pointer to the detector selected by cascade
//!
//-----------------------------------------------------------------------------
const detector_t * get_detector(uint8_t cascade) {
    if (cascade < DETECTOR_TYPE_NUMBER) {
        return &detector_rom[cascade] ;
    }
    else {
        return NULL ;
    }
}


int32_t Cascade_GetPixelValue(const uint32_t *integral_image, uint32_t x,uint32_t y){
	return integral_image[x + y];
}

void Detector_GetCascade(detector_t * detector, int32_t cascadeNo, cascade_t * cascade){
	cascade = &(detector->cascade[cascadeNo]);
}

#define HITEXTRASCORE 20  //Extra score added when there is a hit (level also added)

//-----------------------------------------------------------------------------
//! \brief Executes cascades on local integral image window (LIIW)
//!
//! Executes cascades on local integral image window (LIIW) at position (x,y)
//! with steps (xinc, yinc) using the scanOrder to get the position order where
//! to apply the detector for optimization purpose.
//!
//! \param integral_image 	pointer to the integral image
//! \param x 				X position of the patch to consider
//! \param y 				Y position of the patch to consider
//! \param xinc 			step width to analyze
//! \param yinc 			step height to analyze
//! \param scanOrder 		Array containing the patch coordinate order
//! \param currentCascade 	cascade to consider
//! \param patchWidth		patch width
//! \param wWindow 			Window width
//! \param yOff				Offset of the window on the stripe
//! \param Norm 			Normalization value
//! \param ScalFactor 		scaling factor applied, necessary to rebuild face detected in global list
//! \param pList			Add each rect
//! \return					true if a significant face has been detected
//!
//-----------------------------------------------------------------------------
bool execute_cascade_liiw(const uint32_t * integral_image, uint32_t x, uint32_t y,
		uint32_t xinc, uint32_t yinc, const uint8_t * scanOrder, cascade_t * currentCascade,
		int32_t patchWidth, int32_t patchHeight, int32_t wWindow, int32_t yOff, int32_t Norm,
		int32_t ScalFactor, RectList_t * pList, int8_t cascade, int rotation)
//bool execute_cascade_liiw(const uint32_t * integral_image, const core_param_t * core_param,
//						  const detector_t * detector, uint32_t x, uint32_t y, uint32_t xinc, uint32_t yinc,
//						  int32_t wWindow, int32_t yOff, int32_t Norm,
//						  RectList_t * pList, int8_t cascade, int rotation)
{
    uint32_t sx, sy;
    uint32_t n;
    uint32_t maxhx, maxhy, sumx, sumy;
    result_t result;
    Rect_t * pSol;
    uint32_t PatchScore = 0;
    uint32_t HitCount = 0;
    uint32_t ScoreCond = 0;

//    cascade_t * currentCascade = &(detector->cascade[cascade]);
//    const uint8_t * scanOrder = core_param->scanOrder;
//    int32_t ScalFactor = core_param->ScalFactor;
//    int32_t patchWidth = detector->width - 1;
//    int32_t patchHeight = detector->height - 1;


    int nx[] = {0,0,0,0,0};
    int ny[] = {0,0,0,0,0};

    uint16_t ScanStopScore = 9;
    uint16_t CondLevel1 = 2;
    uint16_t CondLevel2 = 5;
    uint16_t ScoreCond2 = 28;
    uint16_t ScoreCond1 = 24;
    uint16_t ScoreCond0 = 0;

	// Go through the patch window
    // Execute cascade for each position on the patch
    for (n = 0; n < xinc * yinc; n++) {
        sx = scanOrder[2*n];
        sy = scanOrder[2*n+1];
        execute_cascade(integral_image, sx, sy, currentCascade, wWindow, yOff, patchWidth, patchHeight, Norm, &result, rotation);

    	PatchScore += result.level;
        if(result.result){
          	HitCount++;
            nx[sx]++;
            ny[sy]++;
            PatchScore += HITEXTRASCORE;
            if (HitCount > ScanStopScore) {
            	// score sufficient high no additional position should be executed
            	break;
            }
        }

        // Check the score evolution inside the patch
        // If the score evolution is too slow then stop searching at this position
        if (n >= CondLevel1) {
            if (n >= CondLevel2) {
                ScoreCond += ScoreCond2;
            } else {
                ScoreCond += ScoreCond1;
            }
            if (PatchScore*4 < (ScoreCond)) {
            	break;
            }
        }
        else { // Condition level
            ScoreCond += ScoreCond0;
            if (PatchScore*4 < (ScoreCond)) {
            	break;
            }
        }
    } // for n

    // Here we have the ending score level + info if to skip all next cascade
    if (HitCount > 0) {

#ifdef FIND_HIT_ACCURATE_POSITION
        maxhx = 0;
        maxhy = 0;
        sumx = nx[0];
        sumy = ny[0];

        for (n = 1; n < xinc; n++) {
            sumx += nx[n];
            sumy += ny[n];
            maxhx += n*nx[n];
            maxhy += n*ny[n];
        }
        sx = (maxhx + (sumx>>1)) / (sumx);
        sy = (maxhy + (sumy>>1)) / (sumy);
#else
        sx = xinc / 2;
        sy = yinc / 2;
#endif

	pSol = RegisterHit(x + sx,  y + sy, patchWidth, patchHeight, ScalFactor, pList, HitCount, ((rotation-1) << 4 | cascade) );
	
	printf("x+sx = %i, y+sy = %i, HitCount = %i, pSol = %p, score = %i\n", x+sx, y+sy, HitCount, pSol, pSol->score);
 
        if (pSol != NULL) {
            // Then we know this solution get scraped ! and all score test is avoided
            // solution reach required level clear do it and break all pending/on going cacade execution
            // Sufficiently valid, no additional cascade should be executed
            if (pSol->score >= ScanStopScore) {
	      return true;
            }
        }
    } // if HitCount

    return false;
}


//-----------------------------------------------------------------------------
//! \brief Apply the detector at a specific position on local integral image window
//!
//! Apply the detector at a specific position (given by the ScanOrder)
//! for each weak of each level of the cascade as result matches
//!
//! \param integral_image 	pointer to the integral image
//! \param x 				X position on the patch
//! \param y 				Y position on the patch
//! \param currentCascade 	cascade to consider
//! \param wWindow 			Window width
//! \param yOff				Offset of the window on the stripe
//! \param patchWidth		patch width
//! \param Norm 			Normalisation value
//!
//-----------------------------------------------------------------------------
void execute_cascade(const uint32_t * integral_image, uint32_t x, uint32_t y, cascade_t * currentCascade, int32_t wWindow, int32_t yOff, int32_t patchWidth, int32_t patchHeight, int32_t Norm, result_t *pResult, int rotation)
{
  // For optimization reason the first stage of cascades must contain
  // only one weak detector

  weak_t * pWeak;
  int32_t Sigma;
  int32_t AlphaSum;
  int16_t WeakNo;
  int16_t WeaksOffset;
  int16_t nWeak;
  int32_t StrongThr;
  symmetry_e symmetry;

  symmetry = currentCascade->symmetry;

  // Stage 0
  pResult->level = 0;
  WeakNo = currentCascade->stages[pResult->level].WeaksOffset;

  pWeak = currentCascade->weaks + WeakNo;
  pResult->value = Cascade_GetFeatureValue(pWeak, integral_image, wWindow, symmetry, patchWidth, patchHeight, x, y, yOff, rotation);
  pResult->result = Cascade_CheckThreshold(pWeak, pResult->value, Norm);

#define PRINT
#if defined(PRINT) && defined(PULP)
  //printf("currentCascade = 0x%X\n",(unsigned int)currentCascade);
  //printf("currentCascade->stages @ 0x%X\n",(unsigned int)&(currentCascade->stages));
  //printf("currentCascade->stages = 0x%X\n",(unsigned int)(currentCascade->stages));
  //printf("currentCascade->stages[%d].WeaksOffset @ 0x%X\n",pResult->level,(unsigned int)&(currentCascade->stages[pResult->level].WeaksOffset));
  //
  //printf("WeakNo = %d\n", WeakNo);
  printf("pWeak = 0x%X, result = 0x%X, value = 0x%X\n",
         (unsigned int)pWeak, (unsigned int)(pResult->result), (unsigned int)(pResult->value));
#endif 


  if (!pResult->result) {
    return;
  }
  pResult->level++;

  // for each other stage
  while (pResult->level < currentCascade->nb_stage){
    WeakNo = currentCascade->stages[pResult->level].WeaksOffset;
    AlphaSum = currentCascade->stages[pResult->level].AlphaSum;
    WeaksOffset = currentCascade->stages[pResult->level].WeaksOffset;
    nWeak = currentCascade->stages[pResult->level].nWeak;
    StrongThr = currentCascade->stages[pResult->level].StrongThr;
    Sigma = 0;
    do { //for weak
      pWeak = currentCascade->weaks + WeakNo;
      pResult->value = Cascade_GetFeatureValue(pWeak, integral_image, wWindow, symmetry, patchWidth,
					       patchHeight, x, y, yOff, rotation);
      pResult->result = Cascade_CheckThreshold(pWeak, pResult->value, Norm);
      if (pResult->result) {
	Sigma += pWeak->alpha;
	if (Sigma > StrongThr) {
	  break;
	}
      }
      AlphaSum -= pWeak->alpha ;
      if( AlphaSum < (StrongThr-Sigma)) {
	pResult->result = 0;
	return;
      } 
      else {
	pResult->result = 1;
      }

      WeakNo += 1;
    } while (WeakNo < WeaksOffset + nWeak);

    if ( Sigma > StrongThr){
      pResult->result = 1;
    } 
    else {
      pResult->result = 0;
      return;
    }
    pResult->level++;
  }
}


//-----------------------------------------------------------------------------
//! \brief Apply the detector at a specific position on local integral image window
//!
//! Return the value of the the integral value of specific area in function of the type of the weak.
//!
//! \param pWeak	 		@ref weak_t pointer to the current weak of the current cascade at the current level
//! \param integral_image 	pointer to the integral image
//! \param wWindow 			Window width
//! \param symmetry 		symmetry type (none or vertical)
//! \param patchWidth		patch width
//! \param x			 	X position on the patch
//! \param y 				Y position on the patch
//! \param yOff				Offset of the window on the stripe
//! \param yOff				Offset of the window on the stripe
//! \return					result of the calculate in the specific area
//!
//-----------------------------------------------------------------------------
int Cascade_GetFeatureValue(weak_t * pWeak, const uint32_t * integral_image, int32_t wWindow, int32_t symmetry, int32_t patchWidth, int32_t patchHeight, int x, int y, int32_t yOff, int rotation)
{

    int type = 0 ;
    int r = 0 ;
    int c = 0 ;
    int off_r = 0 ;
    int off_c = 0 ;
    int jump_r = 0 ;
    int jump_c = 0 ;
    int tmp = 0 ;
    int value;
    int x0;
    int x1;
    int x2;
    int x3;
    int y0;
    int y1;
    int y2;
    int y3;

    switch (symmetry) {
        case NO_SYMMETRY:
            type = pWeak->type ;
            c = pWeak->c >>2;
            jump_c = pWeak->jump_c >> 2;
            r = pWeak->r >> 2;
            jump_r = pWeak->jump_r >> 2;
            break;

        case V_SYMMETRY:
            type = pWeak->type ;
            c = pWeak->c >>2;
            jump_c = pWeak->jump_c >> 2;
            r = pWeak->r >> 2;
            jump_r = pWeak->jump_r >> 2;
            switch (type) {
                case 0:
                    c = patchWidth - (jump_c + c + 1);
                    break;
                case 1:
                    type = 6;
                    c = patchWidth - (2*jump_c + c + 1);
                    break;
                case 2:
                    c = patchWidth - (3*jump_c + c + 1);
                    break;
                case 3:
                    c = patchWidth - (jump_c + c + 1);
                    break;
                case 4:
                    type = 7;
                    c = patchWidth - (2*jump_c + c + 1);
                    break;
            }
            break;
    }


    switch (rotation) {
        case NORTH: //NO_ROTATION :
            off_r = (y + yOff + r) * wWindow ;
            off_c = x + c;
            jump_r = jump_r * wWindow ;
            break;

        case SOUTH: //R180_ROTATION	  :
            type = pWeak->type;
            switch (type) {
                case 0:
                    type = 5;
                    off_c = patchWidth - (jump_c + c + 1);
                    off_r = patchHeight - (2*jump_r + r + 1);
                    break;
                case 1:
                    type = 6;
                    off_c = patchWidth - (2*jump_c + c + 1);
                    off_r = patchHeight - (jump_r + r + 1);
                    break;
                case 2:
                    off_c = patchWidth - (3*jump_c + c + 1);
                    off_r = patchHeight - (jump_r + r + 1);
                    break;
                case 3:
                    off_c = patchWidth - (jump_c + c + 1);
                    off_r = patchHeight - (3*jump_r + r + 1);
                    break;
                case 4:
                    off_c = patchWidth - (2*jump_c + c + 1);
                    off_r = patchHeight - (2*jump_r + r + 1);
                    break;
                case 5:
                    type = 0;
                    off_c = patchWidth - (jump_c + c);
                    off_r = patchHeight - (2*jump_r + r);
                    break;
                case 6:
                    type = 1;
                    off_c = patchWidth - (2*jump_c + c);
                    off_r = patchHeight - (jump_r + r);
                    break;
                case 7:
                    off_c = patchWidth - (2*jump_c + c);
                    off_r = patchHeight - (2*jump_r + r);
                    break;
            }
            off_r = ((y + yOff + (off_r )) * wWindow);
            off_c = (x + (off_c));
            jump_r = ((jump_r) * wWindow);
            break;

        case EAST: //R90R_ROTATION	  :
            type = pWeak->type;
            switch (type) {
                case 0:
                    type = 1;
                    off_r = c;
                    off_c = patchHeight - (2*jump_r + r + 1);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 1:
                    type = 5;
                    off_r = c;
                    off_c = patchHeight - (jump_r + r + 1);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 2:
                    type = 3;
                    off_r = c;
                    off_c = patchHeight - (jump_r + r + 1);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 3:
                    type = 2;
                    off_r = c;
                    off_c = patchHeight - (3*jump_r + r + 1);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 4:
                    type = 7;
                    off_r = c;
                    off_c = patchHeight - (2*jump_r + r + 1);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 5:
                    type = 6;
                    off_r = c;
                    off_c = patchHeight - (jump_r + r);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 6:
                    type = 0;
                    off_r = c;
                    off_c = patchHeight - (jump_r + r);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 7:
                    type = 4;
                    off_r = c;
                    off_c = patchHeight - (2*jump_r + r);
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
            }
            off_r = ((y + yOff + (off_r )) * wWindow);
            off_c = (x + (off_c));
            jump_r = ((jump_r) * wWindow);
            break;

        case WEST: //R90L_ROTATION:
            type = pWeak->type;
            switch (type) {
                case 0:
                    type = 6;
                    off_r = patchWidth - (jump_c + c + 1);
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 1:
                    type = 0;
                    off_r = patchWidth - (2*jump_c + c + 1)  ;
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 2:
                    type = 3;
                    off_r = patchWidth - (3*jump_c + c + 1) ;
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 3:
                    type = 2;
                    off_r = patchWidth - (jump_c + c + 1);
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 4:
                    type = 7;
                    off_r = patchWidth - (2*jump_c + c + 1);
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 5:
                    type = 1;
                    off_r = patchWidth - (jump_c + c);
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 6:
                    type = 5;
                    off_r = patchWidth - (2*jump_c + c);
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
                case 7:
                    type = 4;
                    off_r = patchWidth - (2*jump_c + c);
                    off_c = r;
                    tmp = jump_r;
                    jump_r = jump_c;
                    jump_c = tmp;
                    break;
            }

            off_r = ((y + yOff + (off_r )) * wWindow);
            off_c = (x + (off_c));
            jump_r = ((jump_r) * wWindow);
            break;
    }


    x0 = off_c + 1; // 1 row more for zeroExtend //x0 = off_c;
    x1 = (x0 + jump_c);
    y0 = off_r + wWindow; // 1 line more for zeroExtend //y0 = off_r;
    y1 = y0 + jump_r;

    //
    //printf("x0 = 0x%X, x1 = 0x%X, y0 = 0x%X, y1 = 0x%X\n",
    //       (unsigned int)x0,(unsigned int)x1,(unsigned int)y0,(unsigned int)y1);
    //printf("type = 0x%X\n",(unsigned int)type);
    //

    switch (type) {
    case 0:
        /* -1 +-----+ +1
         *    |  -  |
         * +2 +-----+ -2
         *    |  +  |
         * -1 +-----+ +1 */
        y2 = y1 + jump_r;
        value =  Cascade_GetPixelValue(integral_image, x0, y1); // Cascade_GetPixelValue(context, x0, y1);
        value -= Cascade_GetPixelValue(integral_image, x1, y1);
        value <<= 1;
        value -= Cascade_GetPixelValue(integral_image, x0, y0);
        value += Cascade_GetPixelValue(integral_image, x1, y0);
        value -= Cascade_GetPixelValue(integral_image, x0, y2);
        value += Cascade_GetPixelValue(integral_image, x1, y2);
        break;
    case 1:
        /* +1   -2    +1
         * +-----+-----+
         * |  +  |  -  |
         * +-----+-----+
         * -1   +2    -1 */
        x2 = (x1 + jump_c);
        value =  Cascade_GetPixelValue(integral_image, x1, y1);
        value -= Cascade_GetPixelValue(integral_image, x1, y0);
        value <<= 1;
        value += Cascade_GetPixelValue(integral_image, x0, y0);
        value -= Cascade_GetPixelValue(integral_image, x0, y1);
        value += Cascade_GetPixelValue(integral_image, x2, y0);
        value -= Cascade_GetPixelValue(integral_image, x2, y1);
        break;
    case 2:
        /* -1   +3    -3    +1
         * +-----+-----+-----+
         * |  -  |  +  |  -  |
         * +-----+-----+-----+
         * +1   -3    +3    -1 */
        x2 = (x1 + jump_c);
        x3 = (x2 + jump_c);
        value =  Cascade_GetPixelValue(integral_image, x1, y0);
        value -= Cascade_GetPixelValue(integral_image, x2, y0);
        value -= Cascade_GetPixelValue(integral_image, x1, y1);
        value += Cascade_GetPixelValue(integral_image, x2, y1);
        value += (value << 1);
        value -= Cascade_GetPixelValue(integral_image, x0, y0);
        value += Cascade_GetPixelValue(integral_image, x0, y1);
        value += Cascade_GetPixelValue(integral_image, x3, y0);
        value -= Cascade_GetPixelValue(integral_image, x3, y1);
        break;

    case 3:
        /* -1 +-----+ +1
         *    |  -  |
         * +3 +-----+ -3
         *    |  +  |
         * -3 +-----+ +3
         *    |  -  |
         * +1 +-----+ -1 */
        y2 = y1 + jump_r;
        y3 = y2 + jump_r;
        value =  Cascade_GetPixelValue(integral_image, x0, y1);
        value -= Cascade_GetPixelValue(integral_image, x0, y2);
        value -= Cascade_GetPixelValue(integral_image, x1, y1);
        value += Cascade_GetPixelValue(integral_image, x1, y2);
        value += (value << 1);
        value -= Cascade_GetPixelValue(integral_image, x0, y0);
        value += Cascade_GetPixelValue(integral_image, x0, y3);
        value += Cascade_GetPixelValue(integral_image, x1, y0);
        value -= Cascade_GetPixelValue(integral_image, x1, y3);
        break;

    case 4:
        /*         +2
         * -1 +-----+-----+ -1
         *    |  -  |  +  |
         * +2 +--- -4 ----+ +2
         *    |  +  |  -  |
         * -1 +-----+-----+ -1
                   +2          */
        x2 = (x1 + jump_c);
        y2 = y1 + jump_r;
        value =  -Cascade_GetPixelValue(integral_image, x1, y1);
        value <<= 1;
        value += Cascade_GetPixelValue(integral_image, x0, y1);
        value += Cascade_GetPixelValue(integral_image, x1, y0);
        value += Cascade_GetPixelValue(integral_image, x1, y2);
        value += Cascade_GetPixelValue(integral_image, x2, y1);
        value <<= 1;
        value -= Cascade_GetPixelValue(integral_image, x0, y0);
        value -= Cascade_GetPixelValue(integral_image, x0, y2);
        value -= Cascade_GetPixelValue(integral_image, x2, y0);
        value -= Cascade_GetPixelValue(integral_image, x2, y2);
        break;
    case 5:
        /* +1 +-----+ -1
         *    |  +  |
         * -2 +-----+ +2
         *    |  -  |
         * +1 +-----+ -1 */
        y2 = y1 + jump_r;
        value =  Cascade_GetPixelValue(integral_image, x1, y1);
        value -= Cascade_GetPixelValue(integral_image, x0, y1);
        value <<= 1;
        value += Cascade_GetPixelValue(integral_image, x0, y0);
        value -= Cascade_GetPixelValue(integral_image, x1, y0);
        value += Cascade_GetPixelValue(integral_image, x0, y2);
        value -= Cascade_GetPixelValue(integral_image, x1, y2);
        break;
    case 6:
        /* -1   +2    -1
         * +-----+-----+
         * |  -  |  +  |
         * +-----+-----+
         * +1   -2    +1 */
        x2 = (x1 + jump_c);
        value =  Cascade_GetPixelValue(integral_image, x1, y0);
        value -= Cascade_GetPixelValue(integral_image, x1, y1);
        value <<= 1;
        value -= Cascade_GetPixelValue(integral_image, x0, y0);
        value += Cascade_GetPixelValue(integral_image, x0, y1);
        value -= Cascade_GetPixelValue(integral_image, x2, y0);
        value += Cascade_GetPixelValue(integral_image, x2, y1);
        break;
    case 7:
        /*         -2
         * +1 +-----+-----+ +1
         *    |  +  |  -  |
         * -2 +--- +4 ----+ -2
         *    |  -  |  +  |
         * +1 +-----+-----+ +1
                   -2          */
        y2 = y1 + jump_r;
        x2 = (x1 + jump_c);
        value =  Cascade_GetPixelValue(integral_image, x1, y1);
        value <<= 1;
        value -= Cascade_GetPixelValue(integral_image, x0, y1);
        value -= Cascade_GetPixelValue(integral_image, x1, y0);
        value -= Cascade_GetPixelValue(integral_image, x1, y2);
        value -= Cascade_GetPixelValue(integral_image, x2, y1);
        value <<= 1;
        value += Cascade_GetPixelValue(integral_image, x0, y0);
        value += Cascade_GetPixelValue(integral_image, x0, y2);
        value += Cascade_GetPixelValue(integral_image, x2, y0);
        value += Cascade_GetPixelValue(integral_image, x2, y2);
        break;
    default:
        value = 0;
    }

    return(value);
}




//-----------------------------------------------------------------------------
//! \brief Check if value is on the range given by Norm.
//!
//! \param pWeak		@ref weak_t pointer to the current weak of the current cascade at the current level
//! \param value	 	value to check
//! \param Norm 		reference value used to determine a range values
//! \return				1 if value is on the good range, else 0.
//!
//-----------------------------------------------------------------------------
int Cascade_CheckThreshold(weak_t * pWeak, int32_t Value, int32_t Norm) {

    int thr2_norm, thr1_norm;

    // over actual 16 bits
    thr2_norm = (pWeak->thr2 * (Norm&0xff)) >> 8;
    thr1_norm = (pWeak->thr1 * (Norm&0xff)) >> 8;
    thr2_norm += (pWeak->thr2 * ((Norm&0xff00)>>8));
    thr1_norm += (pWeak->thr1 * ((Norm&0xff00)>>8));
    thr2_norm >>=8;
    thr1_norm >>=8;

    if (Value >= ((thr2_norm)) || Value <= ((thr1_norm))) {
        return 0;
    } else {
        return 1;
    }
}

