#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//#include <doslib.h>
#include "ff.h"
#include "diskio.h"
#include "xdf.h"

// if we fully include doslib.h, BYTE/WORD type conflict happens
int GETDATE(void);
int GETTIME(void);

static XDF* xdf_ptrs[ XDF_MAX_DRIVES ];

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
  XDF* xdf = xdf_ptrs[ drive ];

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
  XDF* xdf = xdf_ptrs[ drive ];

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

  xdf->fp = fopen(xdf->name, "wb");
  if (xdf->fp == NULL) {
    printf("error: xdf file creation error. (%s)\n", xdf->name);
    goto exit;
  }
  fclose(xdf->fp);

  xdf->fp = fopen(xdf->name, "r+");
  if (xdf->fp == NULL) {
    printf("error: xdf file random open error. (%s)\n", xdf->name);
    goto exit;
  }
  fseek(xdf->fp, XDF_SIZE, SEEK_SET);

  xdf->drive = drive;
  uint8_t drive_letter[] = "0:";
  drive_letter[0] = '0' + xdf->drive;
  f_mount(&xdf->fs, drive_letter, 1);   // format

  xdf_ptrs[ drive ] = xdf;

  rc = 0;

exit:
  return rc;
}

void xdf_close(XDF* xdf) {
  if (xdf != NULL && xdf->fp != NULL) {
    fclose(xdf->fp);
    xdf->fp = NULL;
  }
}
