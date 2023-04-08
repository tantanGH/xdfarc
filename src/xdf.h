#ifndef __H_XDF__
#define __H_XDF__

#include <stdint.h>
#include <stdio.h>

#define XDF_MAX_PATH_LEN  (256)
#define XDF_SECTOR_BYTES  (1024)
#define XDF_NUM_SECTORS   (8)
#define XDF_NUM_TRACKS    (77)
#define XDF_NUM_HEADS     (2)
#define XDF_SIZE_BYTES    (XDF_SECTOR_BYTES * XDF_NUM_SECTORS * XDF_NUM_TRACKS * XDF_NUM_HEADS)

typedef struct {
  uint8_t name[ XDF_MAX_PATH_LEN ];
  FILE* fp;
} XDF;

int32_t xdf_init(XDF* xdf, const uint8_t* xdf_name);
void xdf_close(XDF* xdf);
int32_t xdf_read(XDF* xdf, int32_t sector, size_t len, uint8_t* buf);
int32_t xdf_write(XDF* xdf, int32_t sector, size_t len, uint8_t* buf);

#endif