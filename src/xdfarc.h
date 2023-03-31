#ifndef __H_XDFARC__
#define __H_XDFARC__

#include <stdint.h>
#include <stddef.h>

#define VERSION "0.1.0 (2023/03/31)"

#define MAX_PATH_LEN (256)

#define FD_2HD_BYTES (1248000)

#define FREAD_BUF_BYTES (32768)

typedef struct {
  size_t size_bytes;
  int16_t chunk;
  int16_t num_chunks;
  int16_t destination;
  uint8_t name[ MAX_PATH_LEN ];
  uint8_t path_name[ MAX_PATH_LEN ];
  uint8_t out_name[ MAX_PATH_LEN ];
} FILE_SPLIT;

#endif
