#include <stdio.h>
#include <string.h>

#include "utils.h"

void my_memcpy(void *dst, void *src, unsigned int size)
{
    int j = 0;
    unsigned int *int_dst_ptr = (unsigned int *) dst;
    unsigned int *int_src_ptr = (unsigned int *) src;
    
    for(j = 0; j < size/4; j++)
        int_dst_ptr[j] = (unsigned int) int_src_ptr[j];
}

/* PGM files allow a comment starting with '#' to end-of-line.  
 * Skip white space including any comments. */
static void skip_comments(FILE *fp)
{
    int ch;
    
    fscanf(fp," ");      /* Skip white space. */
    while ((ch = fgetc(fp)) == '#') {
        while ((ch = fgetc(fp)) != '\n'  &&  ch != EOF)
            ;
        fscanf(fp," ");
    }
    ungetc(ch, fp);      /* Replace last character read. */
}

//Read a pgm or ppm file (pgm -> grayscale ppm -> colored)
int readPgmOrPpm(char *filename, raw_image_data_t *raw_image)  
{
    FILE *fl = fopen(filename, "rb");  
    int char1, char2, max, c1, c2, c3;
    
    char1 = fgetc(fl);
    char2 = fgetc(fl);
    skip_comments(fl);
    c1 = fscanf(fl, "%d", &raw_image->width);
    skip_comments(fl);
    c2 = fscanf(fl, "%d", &raw_image->height);
    skip_comments(fl);
    c3 = fscanf(fl, "%d", &max);
        
    if (char1 != 'P' || (char2 != '5' && (char2 != '6')) || 
        c1 != 1 || c2 != 1 || c3 != 1 || max > 255) {
        
        //printf("P %c | %c | c1 %d | c2 %d | c3 %d | max %d\n", char1,char2,c1,c2,c3,max);
        //printf("Input is not a standard raw 8-bit PGM file.\n");
            goto error;
        }
        //printf("P %c | %c | c1 %d | c2 %d | c3 %d | max %d\n", char1,char2,c1,c2,c3,max);
    
    if(char2 == '5')
        raw_image->nr_channels = 1;
    else
        raw_image->nr_channels = 3;
        
    raw_image->data = (uint8_t *)malloc(raw_image->width*raw_image->height*sizeof(uint8_t)*raw_image->nr_channels);
    
    fgetc(fl);  /* Discard exactly one byte after header. */
    fread(raw_image->data, 1, raw_image->width*raw_image->height*raw_image->nr_channels, fl);
    
error:
    fclose(fl);
    return 0;
}

//Write a pgm or ppm file (pgm -> grayscale ppm -> colored)
int writePgmOrPpm(char *filename, raw_image_data_t *raw_image)
{
    FILE *fl = fopen(filename, "wb");
    if(raw_image->nr_channels == 1)
        fprintf(fl, "P5\n%d %d\n255\n", raw_image->width, raw_image->height);
    else
        fprintf(fl, "P6\n%d %d\n255\n", raw_image->width, raw_image->height);
    
    fwrite(raw_image->data, 1, raw_image->width*raw_image->height*raw_image->nr_channels, fl);  

    fclose(fl);  
    return 0;
}
