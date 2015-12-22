/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: list.c.rca 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: list.c.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#include <raid.h>
#include "list.h"


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 return the "x" value according to the scaling factor
//!
//! \param x       	 	@ref int32_t initial value
//! \param ScalFactor   @ref int32_t scaling factor
//! \return         	@ref int_32_t rescaled value
//-----------------------------------------------------------------------------
int32_t ReScalImage(int32_t x, int32_t ScalFactor)
{
	return EXT_FIX_INT(x*ScalFactor + EXT_FIX_ROUND);
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 memory allocation for a new Rect_t and initialize it with an other given Rect_t
//!
//! \param pRect        @ref Rect_t Pointer to the input Rect_t
//! \return             @ref Rect_t New allocated Rect_t
//-----------------------------------------------------------------------------
Rect_t * RectAllocCopy(Rect_t * pRect)
{
	Rect_t * pNew = NULL;
	if(pRect != NULL){
		pNew = (Rect_t *) malloc(sizeof(Rect_t));
		assert(pNew != NULL);
		memset(pNew, 0, sizeof(Rect_t)) ;
		pNew->pNextRect = NULL;
		pNew->x0 = pRect->x0;
		pNew->y0 = pRect->y0;
		pNew->x1 = pRect->x1;
		pNew->y1 = pRect->y1;
		pNew->score = pRect->score;
		pNew->area = pRect->area;
		pNew->Id = pRect->Id;
		pNew->CascadeTag = pRect->CascadeTag;
	}
	return pNew;
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Free memory of the given Rect_t
//!
//! \param pRect        @ref Rect_t Pointer to the Rect_t for free
//-----------------------------------------------------------------------------
void RectFree(Rect_t * pRect){
	if(pRect != NULL){
    	free(pRect);
	}
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Update area parameter of the given Rect_t
//!
//! \param pRect        @ref Rect_t Pointer to the Rect_t for update
//!
//-----------------------------------------------------------------------------
void RectUpdateArea(Rect_t * pRect)
{
	int32_t w,h;
	w = (pRect->x1 - pRect->x0);
	h = (pRect->y1 - pRect->y0);
	pRect->area = w * h;
}

//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Check  the intersection between 2 Rect_t
//!
//! \param pRect1       @ref Rect_t Pointer to the first Rect_t
//! \param pRect2       @ref Rect_t Pointer to the second Rect_t
//! \param pRectInter	@ref Rect_t Pointer to the Rect_t which is the intersection between 2 first
//! \return				@ref e_intersection type of the intersection
//!
//-----------------------------------------------------------------------------
e_intersection RectIntersection(const Rect_t * pRect1, const Rect_t * pRect2, Rect_t * pRectInter)
{
	pRectInter->x0 = MAX(pRect1->x0, pRect2->x0);
	pRectInter->y0 = MAX(pRect1->y0, pRect2->y0);
	pRectInter->x1 = MIN(pRect1->x1, pRect2->x1);
	pRectInter->y1 = MIN(pRect1->y1, pRect2->y1);

    //Do they intersect ?
    if (!(pRectInter->x0 > pRectInter->x1 || pRectInter->y0 > pRectInter->y1)) {
        RectUpdateArea(pRectInter);
        if (pRect1->area <  pRect2->area) {
//            if (pRectInter->area >=  (pRect1->area*12/16))  { // 75 % at least to be considered fully included
			  if (16*pRectInter->area >=  pRect1->area*12 )  { // 75 % at least to be considered fully included
                return R1_FULLY_INCLUDED; // pRect1 is included into pRect2
            } else
            	//if  (pRectInter->area >=  (pRect1->area*8/16)) {
             if ( 2*pRectInter->area >=  pRect1->area) {
            	return R1_HALFLY_INCLUDED; // pRect1 is halfly included into pRect2
            }
        }
		else
		if (pRect1->area >  pRect2->area) {
            //if (pRectInter->area >= (pRect2->area*12/16)) {
			if ( 16*pRectInter->area >= pRect2->area*12 ) {
                return R2_FULLY_INCLUDED; // pRect2 is included into pRect1
            }else
            	if(  2*pRectInter->area >=  pRect2->area ) { //if  (pRectInter->area >=  (pRect1->area*8/16)) {
           		return R2_HALFLY_INCLUDED; // pRect2 is halfly included into pRect1
           	}
		}
        return PARTIAL_INCLUSION;// default, no full inclusion
    }
    return NONE_INCLUSION;

}

//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Initialize the RectList_t
//!
//! \param pRectList        @ref RectList_t Pointer to the RectList_t
//!
//-----------------------------------------------------------------------------
void RectListInit(RectList_t * pRectList)
{
	pRectList->NbRect = 0;
    pRectList->pHeadRect = NULL;
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Free all the elements of the pRectList
//!
//! \param pRectList        @ref RectList_t Pointer to the RectList_t
//!
//-----------------------------------------------------------------------------
void RectListFree(RectList_t * pRectList)
{
	Rect_t * pRect;
    while (pRectList->pHeadRect != NULL) {
    	pRect = pRectList->pHeadRect;
    	pRectList->pHeadRect = pRect->pNextRect;
    	free(pRect);
    }
    pRectList->NbRect = 0;
}

//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Free pRect elements of the pRectList
//!
//! \param pRectList        @ref RectList_t Pointer to the RectList_t
//! \param pRect        	@ref Rect_t Pointer to the element to free
//! \param pPrevRect      	@ref Rect_t Pointer to the element above the element to free
//!
//-----------------------------------------------------------------------------
void RectListFreeRect(RectList_t * pList, Rect_t * pRect, Rect_t * pPrevRect)
{
	if(pRect != NULL){
		if(pRect = pList->pHeadRect){
			pList->pHeadRect = pRect->pNextRect;
			free(pRect);
			pList->NbRect--;
		} else if(pPrevRect != NULL) {
			pPrevRect->pNextRect = pRect->pNextRect;
			free(pRect);
			pList->NbRect--;
		}
	}
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Add p item to the list (do not use if p is null)
//!
//! \param pList        	@ref RectList_t Pointer to the RectList_t
//! \param p        		@ref Rect_t Pointer to the element to add
//!
//-----------------------------------------------------------------------------
void RectListAddItem(RectList_t *pList, Rect_t *p)
{
	if((pList != NULL) && (p != NULL)){
		p->pNextRect = pList->pHeadRect;
		pList->pHeadRect = p;
		pList->NbRect++;
	}
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Add p item to the sort list (largest area first)
//!
//! \param pList        	@ref RectList_t Pointer to the RectList_t
//! \param pAdd        		@ref Rect_t Pointer to the element to add
//!
//-----------------------------------------------------------------------------
void RectListAddSortItem(RectList_t *pList, Rect_t *pAdd)
{
    Rect_t *p;
    Rect_t *pPrev;

    // Added to empty is  starting the list and no sort is required
	if((pList != NULL) && (pAdd != NULL)){
		if (pList->pHeadRect == NULL) {
			// empty list
			pList->pHeadRect = pAdd;
			pAdd->pNextRect = NULL;
			pList->NbRect = 1;

		} else {

			p = pList->pHeadRect;
			pPrev = NULL;

			// SortLargestFirst
			while ((pAdd->area < p->area) && (p->pNextRect != NULL))
			{
				pPrev = p;
                p = p->pNextRect;
			}
			//when last item reach it still must be checked  if to insert before or after it base on condition
			if ((p->pNextRect == NULL)) {
				//insert after last
				p->pNextRect = pAdd; //pAdd->pNext is NULL by first init
				pAdd->pNextRect = NULL;
			} else {
				//Insert before p (may be the last  last)
				pAdd->pNextRect = p;
				if (pPrev == NULL) {
					pList->pHeadRect = pAdd;
				} else {
					pPrev->pNextRect = pAdd;
				}
			}// Insert before position
			//Item added update count
			pList->NbRect++;
		}// else list empty
	}
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	Remove pR element in pList
//!
//! \param pList        	@ref RectList_t Pointer to the RectList_t
//! \param pR        		@ref Rect_t Pointer to the element to remove
//!
//-----------------------------------------------------------------------------
void RectListRemoveItem(RectList_t *pList, Rect_t *pR)
{
    if (pList->pHeadRect != NULL) {

    	if (pR == pList->pHeadRect) {
    		pList->pHeadRect = pR->pNextRect;
    		pList->NbRect--;

    	} else {
    		Rect_t* pp = pList->pHeadRect;
    		//Find list item positing item to current
        	while ((pp != NULL) && (pp->pNextRect != pR))
    			pp = pp->pNextRect;

    		if ((pp != NULL) && (pp->pNextRect != NULL)) {
    			pp->pNextRect = pR->pNextRect;
    			pList->NbRect--;
    		}
    	}//not head
    }
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	Remove pR element in pList and free allocated memory associateds
//!
//! \param pList        	@ref RectList_t Pointer to the RectList_t
//! \param pR        		@ref Rect_t Pointer to the element to remove
//!
//-----------------------------------------------------------------------------
void RectListFreeItem(RectList_t *pList, Rect_t *pR) {
    if (pList->pHeadRect != NULL) {

    	if (pR == pList->pHeadRect) {
    		pList->pHeadRect = pR->pNextRect;
    		pList->NbRect--;
    		free(pR);
    	} else {
    		Rect_t* pp = pList->pHeadRect;
    		//Find list item positing item to curent
    		while (( pp != NULL) && (pp->pNextRect != pR))
    			pp = pp->pNextRect;

    		if (( pp != NULL) && (pp->pNextRect != NULL)) {
    			pp->pNextRect = pR->pNextRect;
    			pList->NbRect--;
    			free(pR);
    		}
    	}//not head
    }
}



//-----------------------------------------------------------------------------
//! \brief
//!
//!	Add pRect in pFaceList and try to merge it
//!
//! \param pFaceList       	@ref RectList_t Pointer to the RectList_t
//! \param pRect       		@ref Rect_t Pointer to the element to add and merge
//! \param MergeScore		@ref int16_t threshold to accept merge
//! \param boIgnoreArea		@ref int16_t test to check or not the Rect_t area
//!
//-----------------------------------------------------------------------------
Rect_t * RectListMergeRect(RectList_t *pFaceList, Rect_t * pRect, int16_t MergeScore, int16_t boIgnoreArea)
{

	Rect_t *pR, *pSolution, *pPrevRect;
    int intersect_res;

    pSolution = pRect;
    pPrevRect = NULL;
    pR = pFaceList->pHeadRect;
    do {
        //just merge rect at same area
        if (boIgnoreArea || (pR->area == pSolution->area)) {
            Rect_t Intersect;
            //ratio big/small  mini acceptable
            int MinSupMax_MinMul = 2;
            int MinSupMax_MaxMul = 1;

            //do solution intersect with item?
            intersect_res = RectIntersection(pR, pSolution, &Intersect);
            if (intersect_res) { // intersection wihtout any full inclusion
                int boMerge = 0;
                int area_factor = 2;
                //build
                if ((intersect_res == R1_FULLY_INCLUDED) || (intersect_res == R2_FULLY_INCLUDED)) { // Full Intersection
                    boMerge = 1;
                } else {
                    //DEBUG(" 5 Intersection is fine, MergeCond!=MergeCondAlways ");
                    Rect_t *pMinRect;
                    Rect_t *pMaxRect;
                    if (pR->area < pSolution->area) {
                        pMinRect = pR;
                        pMaxRect = pSolution;
                    } else {
                        //DEBUG(" 6 list Item area is greater than solution i %d s %d ", pR->area, pSolution->area);
                        pMinRect = pSolution;
                        pMaxRect = pR;
                        //CHECK( pR->area >pSolution->area, "Item area is not greater than solution intersect_res %d area %d", intersect_res, Intersect.area);
                    }
                    boMerge = 0;

                    // If the smallest rect represents at least MinSupMax_MaxMul / MinSupMax_MinMul
                    // of the biggest rect and is included at alt least 50% do the merge
                    if (pMinRect->area*MinSupMax_MinMul >= pMaxRect->area*MinSupMax_MaxMul) {
                        if (Intersect.area*area_factor > pMinRect->area)  {
                            boMerge = 1;
                        }
                    }

                    if (boMerge && MergeScore) {
                        //do not integrate not valid face onto  valid faces !
                        if ((pSolution->score >= MergeScore &&   pR->score  < MergeScore)
                                || (pSolution->score < MergeScore &&   pR->score  >= MergeScore))
                            boMerge = 0;
                        else
                            boMerge = pR->score + pSolution->score > MergeScore;
                    }
                } //merge condition sort

                if (!boMerge && (pSolution->score != pR->score)) {
                    if ((intersect_res == R1_HALFLY_INCLUDED) || (intersect_res == R2_HALFLY_INCLUDED))  { // Full Intersection
                        boMerge = 1;
                        if (pSolution->score > pR->score) {
                            // pR is replaced by pSolution
                            pR->CascadeTag = pSolution->CascadeTag;
                            pR->x0 = pSolution->x0;
                            pR->x1 = pSolution->x1;
                            pR->y0 = pSolution->y0;
                            pR->y1 = pSolution->y1;
                            pR->score = pSolution->score;
                        }
                    }
                }

                if (boMerge) {
                    int bActiveMeanMerge = 0;
                    int bSelectSolution = 0;

                    if (pR->area != pSolution->area) {
                        if (intersect_res == R1_FULLY_INCLUDED)   {
                            // pR is fully included into pSolution
                            // condition to absorb pR with pSolution (and not merge it)
                            // the pSolution score to be at least half of pR score (biggers face always get a lower score than lower)
                            if (2*pSolution->score >= pR->score) {
                                bSelectSolution = 1;
                            } else {
                                bActiveMeanMerge = 1;
                            }
                        } else if (intersect_res == R2_FULLY_INCLUDED) {
                            // pSolution is fully included into pR
                            // condition to absorb pSolution with pR (and not merge it)
                            // the pR score to be at least half of pSolution score (biggers face always get a lower score than lower)
                            if (pSolution->score >= 2*pR->score) {
                                bSelectSolution = 1;
                            } else {
                                bActiveMeanMerge = 1;
                            }
                        }
                    } else {
                        // partial, half inclusion
                        bActiveMeanMerge = 1;
                    }

                    if (bActiveMeanMerge) {
                        int ScoreSum = pR->score + pSolution->score;
                        int HalfRound = ScoreSum / 2;
                        assert(ScoreSum != 0);
                        pR->x0 = ((pR->x0 * pR->score) + (pSolution->x0 * pSolution->score) + HalfRound) / (ScoreSum);
                        pR->x1 = ((pR->x1 * pR->score) + (pSolution->x1 * pSolution->score) + HalfRound) / (ScoreSum);
                        pR->y0 = ((pR->y0 * pR->score) + (pSolution->y0 * pSolution->score) + HalfRound) / (ScoreSum);
                        pR->y1 = ((pR->y1 * pR->score) + (pSolution->y1 * pSolution->score) + HalfRound) / (ScoreSum);
                        if (pSolution->score > pR->score)
                            pR->CascadeTag = pSolution->CascadeTag;
                        pR->score += pSolution->score;
                    } else if (bSelectSolution) {
                        pR->CascadeTag = pSolution->CascadeTag;
                        pR->x0 = pSolution->x0;
                        pR->x1 = pSolution->x1;
                        pR->y0 = pSolution->y0;
                        pR->y1 = pSolution->y1;
                        pR->score = pSolution->score + (pR->score+1)/ 2;
                    }

                    RectUpdateArea(pR);
                    //DEBUG("  Rect_AddSolution merged to @%dx%d S%dx%d Score : %d", pR->x0, pR->y0, pR->x1 - pR->x0, pR->y1 - pR->y0, pR->score);

                    if (pSolution != pRect) {
                        //if not initial rect then free the solution a it merged into pR
                    	RectListFreeItem(pFaceList, pSolution);
                    }
                    //Keep merged out of list
                    RectListRemoveItem(pFaceList, pR);
                    pR->pNextRect = NULL; // terminate pR as it as now removed from list!
                    //rescan from start with new solution as new modified faces may merge with older scanned item
                    //try to remerge it now
                    pSolution = pR; // Here is our new solution

                   //for no multi merge just break instead of re-scan
					pPrevRect = NULL;
                    pR = pFaceList->pHeadRect; //rescan fom start
                    continue;
                }
            }
        }
        pPrevRect = pR;
        pR = pR->pNextRect;

    } while (pR != NULL);

    return pSolution;

}



//-----------------------------------------------------------------------------
//! \brief
//!
//!	check if pRect can be merge, allocate memory if new Rect_t and
//! add it in pFaceList
//!
//! \param pFaceList       	@ref RectList_t Pointer to the RectList_t
//! \param pRect       		@ref Rect_t Pointer to the element to add and merge
//! \param validScore		@ref int16_t threshold to accept merge
//! \param boIgnoreArea		@ref int16_t test to check or not the Rect_t area for merge
//!
//-----------------------------------------------------------------------------
Rect_t * RectListAddMergeItem(RectList_t * pList, Rect_t * pRect, int16_t validScore, int16_t boIgnoreArea )
{
	Rect_t * pSolution;

	assert (pList != NULL);

    RectUpdateArea(pRect);
    pRect->Id = pList->NbRect;

    if (pList->pHeadRect != NULL) { // not empty list, try to merge
    	pSolution = RectListMergeRect(pList, pRect, validScore, boIgnoreArea);
    } else {
    	pSolution = pRect;
    }

    if (pSolution == pRect) {
       	pSolution = RectAllocCopy(pRect);
    }

    if (pSolution != NULL) {
    	RectListAddItem(pList, pSolution);
    }
    return pSolution;
}


//-----------------------------------------------------------------------------
//! \brief
//!
//!	Filter Solution that are located so close each other
//! that only a single solution should be considered.
//!
//! \param pFaceList       	@ref RectList_t Pointer to the RectList_t
//! \param pRect       		@ref Rect_t Pointer to the element to check
//! \return					@ref Rect_t final merged solution
//!
//-----------------------------------------------------------------------------
Rect_t *RectListFilterNearbySolution(RectList_t *pFaceList, Rect_t * pRect) {

    Rect_t *pR, *pSolution, *pPrevRect;
    int intersect_res;

    assert(pRect != NULL);
    assert(pFaceList != NULL);
    //We only want valid rectangle the list as area etc are used afterward
    if (pFaceList->pHeadRect == NULL) {
        //note that ipHeadRectf memory alloc fails pHead is still NULL and list remains empty!
        pFaceList->pHeadRect = RectAllocCopy(pRect);
        if (pFaceList->pHeadRect != NULL) {
            // how could this hapen unless in multi search where all heap will have been consumed by first pass?
            pFaceList->NbRect = 1;
            pFaceList->pHeadRect->pNextRect = NULL;
        }
        return pFaceList->pHeadRect;
    }

    pSolution = pRect;
    pPrevRect = NULL;
    pR = pFaceList->pHeadRect;
    Rect_t Intersect;
    DEBUG("Rect_FilterNearbySolution starting ");
    do {
        //just merge rect at same tag class is asked to
        intersect_res = RectIntersection(pR, pSolution, &Intersect);

        if (intersect_res) { // intersection wihtout any full inclusion
            //if (pSolution->score != pR->score) {
            if ((intersect_res >= PARTIAL_INCLUSION))  { // Full or Half Intersection
                if ((pSolution->score > pR->score) || ( (pSolution->score == pR->score) & (pSolution->area < pR->area))) {
                    // either Solution is greater => Solution taken
                    // or Solution equal to pR and Solution area smaller => Solution taken
                    // in case of similar score, it's better to take the smalest face (have the smalest false positive ni worst case)
                    // select pSolution  is taken, pR will be removed

                    pR->CascadeTag = pSolution->CascadeTag;
                    pR->x0 = pSolution->x0;
                    pR->x1 = pSolution->x1;
                    pR->y0 = pSolution->y0;
                    pR->y1 = pSolution->y1;
                    pR->score = pSolution->score;
                }

                if (pSolution != pRect) {
                    //if not initial rect then free the solution a it merged into pR
                    //Rect_ListItemFree(pSolution);
//                	RectListFreeRect(pFaceList, pSolution, pPrevRect);
                	RectListFreeItem(pFaceList, pSolution);
                }
                //Keep merged out of list
                RectListRemoveItem(pFaceList, pR);
                pR->pNextRect = NULL; // terminate pR as it as now removed from list!
                //rescan from start with new soltuion as new modified faces may merge with older scaned item
                //try to remerge it now
                pSolution = pR; // Here is our new solution
            }
            // }
        }// Tag mismatch
        pPrevRect = pR;
        pR = pR->pNextRect;
    } while (pR != NULL);

    if (pSolution == pRect) {
        //Rect is solution (it didn't merged)
        //So a real dynmaic copie must be allacated before to insert it
        pSolution = RectAllocCopy(pRect);
        //This can fail an return NULL !!!!
    }

    if (pSolution != NULL) {
        pSolution->pNextRect = pFaceList->pHeadRect;
        pFaceList->pHeadRect = pSolution;
        pFaceList->NbRect++;
    }

    return pSolution;
} // RectAdd_solution_final



//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Concat pL1 and pL2 to create pL
//!
//! \param pL        	@ref RectList_t Pointer to the final RectList_t
//! \param pL1        	@ref RectList_t Pointer to the first RectList_t
//! \param pL2      	@ref RectList_t Pointer to the second RectList_t
//!
//-----------------------------------------------------------------------------
void RectListConcat(RectList_t *pL , RectList_t *pL1, RectList_t *pL2)
{
    Rect_t *p;
    RectList_t *pLong;
    RectList_t *pShort;

    if (pL2->NbRect < pL1->NbRect) {
        pLong = pL1;
        pShort = pL2;
    } else {
        pLong = pL2;
        pShort = pL1;
    }

    if (pShort->pHeadRect != NULL) {
        p = pShort->pHeadRect;
        while (p->pNextRect != NULL) //or for i .to nrect-1 but still have to walk the list !
            p = p->pNextRect;
        //link the list
        p->pNextRect = pLong->pHeadRect;
        // make pShort resulting head
        p = pShort->pHeadRect;
    } else
        p = pLong->pHeadRect; // the 1st merge item is Longest list start
    //nothing to do when short list as no item just endlist is long
    pL->NbRect = pShort->NbRect + pLong->NbRect;
    pL->pHeadRect = p;
}

//-----------------------------------------------------------------------------
//! \brief
//!
//!	 Add a Rect_t in the detected face list
//!
//! \param x        	@ref int32_t x coordinate of the face
//! \param y        	@ref int32_t y coordinate of the face
//! \param length      	@ref int32_t length of the face
//! \param pList       	@ref RectList_t Pointer to the face list
//! \param score       	@ref int32_t Score of the detection
//! \param cascade     	@ref int32_t Cascade of the detection
//! \return      		@ref Rect_t Pointer to the Rect_t of the face
//!
//-----------------------------------------------------------------------------
Rect_t * RegisterHit(int32_t x, int32_t y, int32_t width, int32_t length, int32_t ScalFactor, RectList_t * pList, const int score, const int cascade)
{
	Rect_t rect;

	// resize hit coordinates in full area
	rect.x0 = ReScalImage(x, ScalFactor);
	rect.y0 = ReScalImage(y, ScalFactor);
	rect.x1 = rect.x0 + ReScalImage(width+1, ScalFactor);
	rect.y1 = rect.y0 + ReScalImage(length+1, ScalFactor);
	rect.score = score;
	rect.CascadeTag = cascade;

	return RectListAddMergeItem(pList, &rect, 0, 0);
}


//-----------------------------------------------------------------------------
//! \brief
//!
//! merge all rect ignoring class and applying class merge score condition
//! Item with score below mClass merge are left in list but are not merge
//! so if spool list is N then make sure search use at most N-1
//! for final merge to work
//! TODO if we finaly do not condition score merge (always or never) then
//! final merge and sweep can be done in a single list scan pass
//!
//! \param pResult     	@ref RectList_t Pointer to the list to analyze
//! \param ValidScore  	@ref int32_t Score of the detection
//!
//-----------------------------------------------------------------------------
void FinalMerge(RectList_t *pResult, int16_t validScore)
{
    RectList_t Final; // will build up a final list by pushing (with merge) item from result
    RectList_t NotMerging;
    Rect_t *p;
    int ClassMergeScore = 2;

    if (pResult->pHeadRect == NULL)
        return;

   	RectListInit(&Final);
	RectListInit(&NotMerging);

	while (pResult->pHeadRect != NULL) {
		p = pResult->pHeadRect;
		RectListRemoveItem(pResult, pResult->pHeadRect);

		if (p->score >= ClassMergeScore  ) {
			RectListAddMergeItem(&Final, p, validScore, 1);
			// *p got duplicated for insertion so delete !
			RectFree(p);
		} else
			RectListAddItem(&NotMerging,  p);
	}
	//group the two list

	RectListConcat(pResult, &Final, &NotMerging);
    RectListInit(&Final);
    RectListInit(&NotMerging);
    DEBUG("Executing FIlterNearbySolution ");
    while (pResult->pHeadRect != NULL) {
		p = pResult->pHeadRect;
		RectListRemoveItem(pResult, pResult->pHeadRect);
        RectListFilterNearbySolution(&Final, p);
        RectFree(p);
    }
    RectListConcat(pResult, &Final, &NotMerging);
}


//-----------------------------------------------------------------------------
//! \brief
//!
//! Build the final list from the current list
//!
//! \param pResult   	  	@ref RectList_t Pointer to the current list
//! \param pFinalList     	@ref RectList_t Pointer to the final list
//! \param ValidScore  		@ref int32_t Score of the detection
//! \param KeepMinScore		@ref int32_t Min Score of the detection
//!
//-----------------------------------------------------------------------------
void ValidateResultList(RectList_t *pResult, RectList_t *pFinalList, int16_t validScore, int16_t KeepMinScore)
{
//	printf("validate result\n");
    FinalMerge(pResult, validScore);

    //Sweep from score
    while (pResult->pHeadRect != NULL) {

        Rect_t *p;

        //move out head item of list
		p = pResult->pHeadRect;
		RectListRemoveItem(pResult, p);
//    	printf("\t\tp.x:%d, p.y:%d, p.w:%d, p->score:%d\n", p->x0, p->y0, p->x1 - p->x0, p->score);

        if (p->score >= validScore) { //KeepMinScore ) {
//        	printf("keep:\t\tp.x:%d, p.y:%d, p.w:%d, p->score:%d\n", p->x0, p->y0, p->x1 - p->x0, p->score);
        	RectListAddSortItem(pFinalList, p);
        	//RectListAddItem(pFinalList, p);
        } else {
            // score too low scrap item
//        	printf("free:\t\tp.x:%d, p.y:%d, p.w:%d, p->score:%d\n", p->x0, p->y0, p->x1 - p->x0, p->score);
        	RectFree(p);
        }
    }//while head
}


