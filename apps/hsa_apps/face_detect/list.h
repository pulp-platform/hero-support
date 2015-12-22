/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * $Id: list.h.rca 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: list.h.rca $
 * 
 *  Revision: 1.1 Tue Oct 16 13:56:53 2012 odile.rey@st.com
 *  Initial version
 ******************************************************************************/
#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C"
{
#endif


/*
typedef struct {
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
} object_t;


typedef struct olist_t {
    object_t object;
    struct olist_t * next;
} object_list_t;
*/



#define MAXRECTINLIST 1024

#define EXT_FIX_PREC 16
#define EXT_FIX_INT(x) ((x>>EXT_FIX_PREC))
#define EXT_FIX_ROUND (1<<(EXT_FIX_PREC - 1))


typedef struct Rect_t {
	
	int16_t x0, y0, x1, y1;
	int16_t score;
	int32_t area;
	int8_t Id;
	char CascadeTag;
	struct Rect_t * pNextRect;
// not enough used    int16_t bMisc ; /* bit field use to various  bit 0 = valid on it's basic search cond pas1/pas2*/
} Rect_t;

typedef struct {
	int32_t NbRect;
	Rect_t * pHeadRect;
} RectList_t;

typedef enum
{
	NONE_INCLUSION,
	PARTIAL_INCLUSION,
	R1_FULLY_INCLUDED,
	R2_FULLY_INCLUDED,
	R1_HALFLY_INCLUDED,
	R2_HALFLY_INCLUDED

} e_intersection;


extern Rect_t * RectAllocCopy(Rect_t * pRect);
extern void RectFree(Rect_t * pRect);
extern void RectUpdateArea(Rect_t * pRect);
extern e_intersection RectIntersection(const Rect_t * pRect1, const Rect_t * pRect2, Rect_t * pRectInter);

extern void RectListInit(RectList_t * pRectList);
extern void RectListFree(RectList_t * pRectList);
extern void RectListAddItem(RectList_t *pList, Rect_t *p);
extern Rect_t * RectListAddMergeItem(RectList_t * pList, Rect_t * pRect, int16_t MergeScore, int16_t boIgnoreArea);
extern void RectListRemoveItem(RectList_t *pList, Rect_t *pR);
extern void RectListFreeItem(RectList_t *pList, Rect_t *pR);
extern void RectListMerge(RectList_t *pL , RectList_t *pL1, RectList_t *pL2);

extern void ValidateResultList(RectList_t *pResult, RectList_t *pFinalList, int16_t validScore, int16_t keepScore);
extern void FinalMerge(RectList_t *pResult, int16_t validScore);

extern Rect_t * RegisterHit(int32_t x, int32_t y, int32_t width, int32_t length, int32_t ScalFactor, RectList_t * pList, const int cascade, const int score);

extern int32_t ReScalImage(int32_t x, int32_t ScalFactor);


#ifdef __cplusplus
}
#endif

#endif // __LIST_H__









