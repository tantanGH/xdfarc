#ifndef __H_XDF__
#define __H_XDF__

#include <stdint.h>
#include <stdio.h>

#define XDF_MAX_PATH_LEN (256)

#define XDF_SIZE (1261568)  // 1024 * 8 * 77 * 2

#define XDF_MAX_DRIVES (10)

#define SECTOR_SIZE (512)

// DIR_Nameフィールドの先頭バイトDIR_Name[0]は、そのエントリの状態を示す重要なデータです。
// この値が0xE5または0x00の場合は、そのエントリは空で新たに使用可能です。
// それ以外の値の場合は、そのエントリは使用中です。値が0x00のときはテーブルの終端を示し、
// それ以降のエントリも全て0x00であることが保証されるので、無駄な検索を省くことができます。
// ファイル名の先頭バイトが0xE5になる場合は、代わりに0x05をセットします。

// DIR_Nameフィールドは11バイトの文字列で、本体と拡張子の8+3バイトの文字列として格納されます。
// その際、本体と拡張子を区切るドットは除去されます。本体8バイト、拡張子3バイトに満たない場合は、
// それぞれ残りの部分にスペース(0x20)が詰められます。文字コードには、ローカルなコードセット(日本語の場合はCP932 Shift_JIS)が使われます。

typedef struct {
  


} FAT;

typedef struct {
  uint8_t name[11 + 1];
  uint8_t attr;
  uint8_t nt_res;
  uint8_t crt_time_tenth;
  uint16_t crt_time;
  uint16_t crt_date;
  uint16_t lst_acc_date;
  uint16_t fst_clus_hi;
  uint16_t wrt_time;
  uint16_t wrt_date;  
  uint16_t fst_clus_lo;
  uint32_t file_size;
} DIR_ENT;

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