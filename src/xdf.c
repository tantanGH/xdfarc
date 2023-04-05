#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <doslib.h>
#include "ff.h"
#include "diskio.h"
#include "xdf.h"

static XDF* g_xdf_ptrs[ XDF_MAX_DRIVES ];

// 0x000000 - 0x0001ff まで 512 bytes
static uint8_t DISK_IPL[] = {
  0x60,0x3c,0x90,0x58,0x36,0x38,0x49,0x50,0x4c,0x33,0x30,0x00,0x04,0x01,0x01,0x00,  //00000000  `<森68IPL30.....
  0x02,0xc0,0x00,0xd0,0x04,0xfe,0x02,0x00,0x08,0x00,0x02,0x00,0x00,0x00,0x00,0x00,  //00000010  .ﾀ.ﾐ............
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,  //00000020  ...........     
  0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,  //00000030        FAT12   O.
  0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,  //00000040  .ﾀM..ｸK..潛..鵑.
  0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,  //00000050  . N廃晒O~p瓸察&:
  0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,  //00000060  .."N$:..2.N蒜("N
  0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,  //00000070  2:.. IE..xp.... 
  0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,  //00000080  ｱ.Vﾈ..g8ﾒ.. Qﾉ.襴
  0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,  //00000090   ..濮.E...`.E...
  0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,  //000000a0  `.E..(a..."JL...
  0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,  //000000b0  p#NON.2.pONOp.NO
  0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,  //000000c0  t.4)..畛ﾔz.､...怱
  0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,  //000000d0   z.披.d..ﾂ..HBR.
  0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,  //000000e0  "N&:.~2.N.4|h."N
  0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,  //000000f0  .YHUfｦT卸ﾙfｦ/. Y
  0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,  //00000100  ﾑﾙ/./.2|gﾀv@ﾖ.N.
  0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,  //00000110  ".$."_J.f..|ﾕﾂS‘
  0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,  //00000120   .B.`.NﾑpFNO....
  0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,  //00000130  f.p.Nup!NONur.p"
  0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,  //00000140  NOr.t.p#NOa.r.t.
  0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,  //00000150  p#NOv,r p NOQﾋ..
  0x4e,0x75,0x00,0x00,0x04,0x00,0x03,0x00,0x00,0x06,0x00,0x08,0x00,0x1f,0x00,0x09,  //00000160  Nu..............
  0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,  //00000170  ..."..Human.sys 
  0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,  //00000180  が 見つかりません
  0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,  //00000190   ..%..ディスクが
  0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,  //000001a0  　読めません...#
  0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,  //000001b0  ..Human.sys が 壊
  0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,  //000001c0   れています.. ..
  0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,  //000001d0  Human.sys の アド
  0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,  //000001e0   レスが異常です.
  0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00,  //000001f0  human   sys.....
};

// 0x000400 - 0x000402 に 0xfe, 0xff, 0xff の3バイトが入る
// BPB_FatSz16 が 2なので 2クラスタ = 2セクタ = 2 * 1024 = 0x800 が FAT1つ分のサイズ

// 0x000C00 - 0x000C02 に 0xfe, 0xff, 0xff の3バイトが入る
// BPB_NumFATs = 0x02 なので 二重化されている
static uint8_t DISK_FAT_HEADER[] = { 0xfe, 0xff, 0xff };

// 0x0013ff までで 1 + 4 セクタ = 5 * 1024 bytes = 5120 bytes = 0x001400 bytes

// 0x002bff (11263) までルートディレクトリ 0x00 埋め
// 24 * 256 = 6 * 1024 = 6セクタ

// 0x133fff (1261567) まで 0xe5 埋め 1261567 - 11263 = 1250304 = 2442 * 512 = 1221 * 1024
// DataSectors = 1221

static uint8_t DISK_DATA_FILL[] = {
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,

  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
  0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,0xe5,
};

DSTATUS disk_status(BYTE drive) {
  // This function returns the status of the disk.
  // In this example, we'll always return STA_OK to indicate that the disk is ready.
  return RES_OK;
}

DSTATUS disk_initialize(BYTE drive) {
  // This function initializes the disk.
  // In this example, we'll do nothing because the disk is a raw image file on the host filesystem.
  return RES_OK;
}

DRESULT disk_read(BYTE drive, BYTE *buffer, DWORD sector, UINT count) {

  // This function reads data from the disk.
  // In this example, we'll read data from the specified sector of the floppy disk image file.
  FIL file;
  FRESULT res;

  // XDF pointer
  XDF* xdf = g_xdf_ptrs[ drive ];

  // Seek to the specified sector
  fseek(xdf->fp, sector * SECTOR_SIZE, SEEK_SET);

  // Read the data from the file
  size_t bytes_read = fread(buffer, 1, count * SECTOR_SIZE, xdf->fp);

  // Make sure we read the correct number of bytes
  if (bytes_read != count * SECTOR_SIZE) {
    return RES_ERROR;
  }

  return RES_OK;
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, DWORD sector, UINT count) {

  // This function writes data to the disk.
  // In this example, we'll write data to the specified sector of the floppy disk image file.
  FIL file;
  FRESULT res;

  // XDF pointer
  XDF* xdf = g_xdf_ptrs[ drive ];

  // Seek to the specified sector
  fseek(xdf->fp, sector * SECTOR_SIZE, SEEK_SET);

  // Read the data from the file
  size_t bytes_written = fwrite(buffer, 1, count * SECTOR_SIZE, xdf->fp);

  // Make sure we wrote the correct number of bytes
  if (bytes_written != count * SECTOR_SIZE) {
    return RES_ERROR;
  }

  return RES_OK;
}

DRESULT disk_ioctl(BYTE drive, BYTE cmd, void *buff) {
  // This function handles disk-specific control commands.
  switch (cmd) {
    case CTRL_SYNC:
      // Wait for any pending write operations to complete
      return RES_OK;
    case GET_SECTOR_COUNT:
      // Return the total number of sectors on the disk
      *(DWORD*)buff = 16 * 77 * 2;
      return RES_OK;
    case GET_SECTOR_SIZE:
      // Return the sector size of the disk
      *(WORD*)buff = SECTOR_SIZE;
      return RES_OK;
    case GET_BLOCK_SIZE:
      // Return the erase block size of the disk (0 for a floppy disk)
      *(DWORD*)buff = 0;
      return RES_OK;
    default:
      // Unsupported command
      return RES_PARERR;
  }
}

DWORD get_fattime() {

  uint32_t cur_date = GETDATE();
  uint32_t cur_time = GETTIME();

  uint16_t cur_day = cur_date & 0x1f;
  uint16_t cur_mon = (cur_date >> 5) & 0x0f;
  uint16_t cur_year = (cur_date >> 9) & 0x7f;

  uint16_t cur_sec = (cur_time & 0x1f) * 2;
  uint16_t cur_min = (cur_time >> 5) & 0x3f;
  uint16_t cur_hour = (cur_time >> 11) & 0x1f;

  uint32_t fattm = (cur_year << 25) | (cur_mon << 21) | (cur_day << 16) | 
                   (cur_hour << 11) | (cur_min << 5) | cur_sec;

  return fattm;
}

int32_t xdf_init(XDF* xdf, const uint8_t* xdf_name, uint8_t drive) {

  int32_t rc = -1;

  strcpy(xdf->name, xdf_name);
  xdf->written_bytes = 0;
  xdf->drive = drive;
  sprintf(xdf->drive_letter, "%d:", xdf->drive);
  printf("drive_letter=%s\n",xdf->drive_letter);

  g_xdf_ptrs[ xdf->drive ] = xdf;

  xdf->fp = fopen(xdf->name, "wb");
  if (xdf->fp == NULL) {
    printf("error: xdf file creation error. (%s)\n", xdf->name);
    goto exit;
  }

  // 0x000000 - 0x0001ff まで 512 bytes
  fwrite(DISK_IPL, 1, 512, xdf->fp);

  // 一旦閉じる
  fclose(xdf->fp);

  // 再度ランダムアクセスオープン
  xdf->fp = fopen(xdf->name, "rb+");
  if (xdf->fp == NULL) {
    printf("error: xdf file random open error. (%s)\n", xdf->name);
    goto exit;
  }

  // 0x000400 - 0x000402 に 0xfe, 0xff, 0xff の3バイトが入る
  // 0x000C00 - 0x000C02 に 0xfe, 0xff, 0xff の3バイトが入る
  fseek(xdf->fp, 0x000400, SEEK_SET);
  fwrite(DISK_FAT_HEADER, 1, 3, xdf->fp);
  fseek(xdf->fp, 0x000C00, SEEK_SET);
  fwrite(DISK_FAT_HEADER, 1, 3, xdf->fp);

  // 0x002bff まで 0x00 埋め
  fseek(xdf->fp, 0x002c00, SEEK_SET);

  // 0x133fff まで 0xe5 埋め
  for (int16_t i = 0; i < 2442; i++) {
    fwrite(DISK_DATA_FILL, 1, 512, xdf->fp);
  }

//  MKFS_PARM mkfs_opt = { 0 };
//  mkfs_opt.fmt = FM_FAT;
//  mkfs_opt.n_fat = 2;
//  FRESULT fr = f_mkfs(xdf->drive_letter, &mkfs_opt, xdf->ff_work, FF_MAX_SS);
//  printf("f_mkfs fr=%d\n",fr);

  FRESULT fr = f_mount(&xdf->fs, xdf->drive_letter, 1);
  printf("f_mount fr=%d\n",fr);

  FIL fil;
  UINT written_bytes;
  uint8_t n[128];
  sprintf(n, "%s/%s", xdf->drive_letter, "HOGEHOGE.TXT");
  fr = f_open(&fil, n, FA_CREATE_ALWAYS | FA_WRITE);
  printf("f_open fr=%d\n",fr);
  fr = f_write(&fil, "hogehoge\r\n", 10, &written_bytes);
  printf("f_write fr=%d\n",fr);
  fr = f_close(&fil);
  printf("f_close fr=%d\n",fr);

//  fr = f_mount(0, xdf->drive_letter, 0);
//  printf("f_mount fr=%d\n",fr);

  fr = f_open(&fil, n, FA_READ);
  printf("f_open fr=%d\n",fr);
  uint8_t buf[32];
  UINT read_bytes;
  fr = f_read(&fil, buf, 10, &read_bytes);
  printf("f_read fr=%d\n",fr);
  buf[read_bytes + 1] = '\0';
  printf("buf=%s\n",buf);
  fr = f_close(&fil);
  printf("f_close fr=%d\n",fr);    

  rc = 0;

exit:
  return rc;
}

void xdf_close(XDF* xdf) {
  if (xdf != NULL && xdf->fp != NULL) {
    f_mount(0, xdf->drive_letter, 0);
    fclose(xdf->fp);
    xdf->fp = NULL;
  }
}
