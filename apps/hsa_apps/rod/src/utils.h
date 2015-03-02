#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdlib.h>
#include <stdint.h>

typedef struct raw_image_data {
    uint8_t * data;
    uint32_t  width;
    uint32_t  height;
    uint32_t  nr_channels;
} raw_image_data_t;

int readPgmOrPpm(char *filename, raw_image_data_t *raw_image);
int writePgmOrPpm(char *filename, raw_image_data_t *raw_image);
void my_memcpy(void *dst, void *src, unsigned int size);
#endif
