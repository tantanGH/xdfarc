#ifndef __H_FAT12__
#define __H_FAT12__

#include <stdint.h>
#include <stddef.h>
#include "xdf.h"

#define FAT12_DIR_ATTR_READ_ONLY      (0x01)
#define FAT12_DIR_ATTR_HIDDEN         (0x02)
#define FAT12_DIR_ATTR_SYSTEM         (0x04)
#define FAT12_DIR_ATTR_VOLUME_ID      (0x08)
#define FAT12_DIR_ATTR_DIRECTORY      (0x10)
#define FAT12_DIR_ATTR_ARCHIVE        (0x20)
#define FAT12_DIR_ATTR_LONG_FILE_NAME (0x0f)

typedef struct {
  uint8_t name[11];
  uint8_t attr;
  uint8_t name2[10];
  uint16_t wrt_time;
  uint16_t wrt_date;  
  uint16_t fst_clus_lo;
  uint32_t file_size;
} FAT12_DIR_ENTRY;

#define FAT12_FAT0     (0xffe)
#define FAT12_FAT1     (0xfff)
#define FAT12_UNUSED   (0x000)
#define FAT12_RESERVED (0x001)
#define FAT12_FAIL     (0xff7)
#define FAT12_EOC      (0xfff)

#define FAT12_NUM_ALLOCATIONS       (1232)
#define FAT12_NUM_ROOT_DIR_ENTRIES  (192)    // 6 * 1024 / 32 = 192

typedef struct {
  XDF* xdf;
  uint8_t alloc_table[ XDF_SECTOR_BYTES * 2 ];
  uint8_t root_dir[ XDF_SECTOR_BYTES * 6 ];
} FAT12;

int32_t fat12_init(FAT12* fat, XDF* xdf);
void fat12_close(FAT12* fat);

int32_t fat12_flush(FAT12* fat);

int16_t fat12_get_allocation(FAT12* fat, int16_t entry_index);
void fat12_set_allocation(FAT12* fat, int16_t entry_index, int16_t new_allocation);

int32_t fat12_find_free_clusters(FAT12* fat, int16_t num_clusters, int16_t* clusters);

void fat12_create_dir_entry(FAT12* fat, FAT12_DIR_ENTRY* dir_ent, uint8_t* file_name, uint8_t attr, uint16_t file_date, uint16_t file_time, uint32_t file_size, int16_t first_cluster);

int32_t fat12_add_root_dir_entry(FAT12* fat, FAT12_DIR_ENTRY* dir_ent);
int32_t fat12_add_dir_entry(FAT12* fat, int16_t current_dir_cluster, FAT12_DIR_ENTRY* dir_ent);

int32_t fat12_read_cluster(FAT12* fat, int16_t cluster, uint8_t* buf);
int32_t fat12_write_cluster(FAT12* fat, int16_t cluster, uint8_t* buf);

#endif