/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                              Imaging Division
 *------------------------------------------------------------------------------
 * %Id%
 *------------------------------------------------------------------------------
 * %Log%
 ******************************************************************************/

#include "raid.h"
#include "detect.h"
#include "release.h"
#include "lena.c"

int main(int argc, const char *argv[])
{
    const char *input = "lena4.raw";
    IMG_image_t inImage;
    detect_param_t param;
    
    param.rotation = 1;
    param.xStep = -1;
    param.yStep = -1;
    param.validScore = 5;
    param.iiStripeWidth = 32;
    param.scalingStep = 0.85;
    param.cascade = 0;    

    IMG_Create(&inImage,WIDTH,HEIGHT,IMG_TYPE_GREY,8);
    
    //Load Image
    int i,j;
    for (i = 0; i < WIDTH; ++i)
        for (j = 0; j < HEIGHT; ++j)
            inImage.data[i*WIDTH+j] = rowImage[i*WIDTH+j];
    
    printf("Processing fDetect\n");
    detect(&inImage, &param);
    
    /// Results:
    printf("%d faces detected in image: %s\n", param.list.NbRect, input);
    Rect_t * pRect = param.list.pHeadRect;
    int x, y, w, h;
    while(pRect != NULL){
        x = (pRect->x1-pRect->x0)/2+pRect->x0;
        y = (pRect->y1-pRect->y0)/2+pRect->y0;
        w = pRect->x1-pRect->x0;
        h = pRect->y1-pRect->y0;
        printf("Face:   x:%d, \ty:%d, \tw:%d, \th:%d, \tscore:%d\n", x, y, w, h, pRect->score);
        pRect = pRect->pNextRect;
    }
    return 0;
}
