#ifndef __H_XDF__
#define __H_XDF__

#include <stdint.h>
#include <stdio.h>
#include "ff.h"

#define XDF_MAX_PATH_LEN (256)

#define XDF_SIZE (1261568)  // 1024 * 8 * 77 * 2

#define XDF_MAX_DRIVES (10)

#define SECTOR_SIZE (512)

typedef struct {
  uint8_t name[ XDF_MAX_PATH_LEN ];
  FILE* fp;
  size_t written_bytes;
  uint8_t drive;
  uint8_t drive_letter[8];
  uint8_t ff_work[ FF_MAX_SS ];
  FATFS fs;
} XDF;

int32_t xdf_init(XDF* xdf, const uint8_t* xdf_name, uint8_t drive);
void xdf_close(XDF* xdf);

#endif