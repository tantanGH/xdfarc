#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <jstring.h>
#include <doslib.h>
#include "xdf.h"
#include "fat12.h"

int32_t fat12_init(FAT12* fat, XDF* xdf) {

  int32_t rc = -1;

  fat->xdf = xdf;

  memset(fat->alloc_table, 0, sizeof(uint8_t) * XDF_SECTOR_BYTES * 2);
  memset(fat->root_dir, 0, sizeof(FAT12_DIR_ENTRY) * FAT12_NUM_ROOT_DIR_ENTRIES);

  fat12_set_allocation(fat, 0, FAT12_FAT0);
  fat12_set_allocation(fat, 1, FAT12_FAT1);

  rc = 0;

exit:
  return rc;
}

void fat12_close(FAT12* fat) {
  if (fat != NULL && fat->xdf != NULL) {
    fat12_flush(fat);
  }
}

int32_t fat12_flush(FAT12* fat) {

  int32_t rc = 0;

  // allocation table
  rc = xdf_write(fat->xdf, 1, 2, fat->alloc_table);

  // root directory
  rc += xdf_write(fat->xdf, 5, 6, fat->root_dir);

  return rc;
}

int16_t fat12_get_allocation(FAT12* fat, int16_t entry_index) {

  int16_t ofs = ( entry_index / 2 ) * 3;

  int16_t allocation = (entry_index & 0x01) ?
    (fat->alloc_table[ofs + 1] & 0xf0) + (fat->alloc_table[ofs + 2]) :
    (fat->alloc_table[ofs + 0]) + (fat->alloc_table[ofs + 1] & 0x0f);

  return allocation;
}

void fat12_set_allocation(FAT12* fat, int16_t entry_index, int16_t new_allocation) {

  int16_t ofs = ( entry_index / 2 ) * 3;

  if (entry_index & 0x01) {
    fat->alloc_table[ofs + 1] = ((new_allocation << 4) & 0xf0) + (fat->alloc_table[ofs + 1] & 0x0f);
    fat->alloc_table[ofs + 2] = (new_allocation >> 4) & 0xff;
  } else {
    fat->alloc_table[ofs + 0] = new_allocation & 0xff;
    fat->alloc_table[ofs + 1] = (fat->alloc_table[ofs + 1] & 0xf0) + ((new_allocation >> 8) & 0x0f);
  }
}

int32_t fat12_find_free_clusters(FAT12* fat, int16_t num_clusters, int16_t* clusters) {

  int16_t found = 0;

  for (int16_t i = 2; i < FAT12_NUM_ALLOCATIONS - 11; i++) {
    int16_t allocation = fat12_get_allocation(fat, i);
    if (allocation == 0x00) {
      clusters[ found++ ] = i;
      if (found >= num_clusters) {
        break;
      }
    }
  }

  return (found < num_clusters) ? -1 : found;
}

void fat12_create_dir_entry(FAT12* fat, FAT12_DIR_ENTRY* dir_ent, uint8_t* file_name, uint8_t attr, uint16_t file_date, uint16_t file_time, uint32_t file_size, int16_t first_cluster) {

  memset(dir_ent, ' ', 11);
  memset(dir_ent + 11, 0, sizeof(FAT12_DIR_ENTRY) - 11);

  if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
    memset(dir_ent->name, ' ', 11);
    memcpy(dir_ent->name, file_name, strlen(file_name));
  } else {
    uint8_t* c = jstrchr(file_name, '.');
    uint16_t base_name_len = (c != NULL) ? c - file_name : strlen(file_name);
    uint8_t* file_ext = (c != NULL) ? c + 1 : NULL;
    uint16_t file_ext_len = (file_ext != NULL) ? strlen(file_ext) : 0;

    memset(dir_ent->name, ' ', 11);
    memcpy(dir_ent->name, file_name, base_name_len);
    if (file_ext != NULL) {
      memcpy(dir_ent->name + 8, file_ext, file_ext_len > 3 ? 3 : file_ext_len); 
    }
    if (base_name_len > 8) {
      memcpy(dir_ent->name2, file_name + 8, base_name_len - 8);
    }
  }

  dir_ent->attr = attr;
  dir_ent->wrt_time = file_time;
  dir_ent->wrt_date = file_date;
  dir_ent->fst_clus_lo = first_cluster;
  dir_ent->file_size = (dir_ent->attr & FAT12_DIR_ATTR_DIRECTORY) ? 0 : file_size;
}

int32_t fat12_add_root_dir_entry(FAT12* fat, FAT12_DIR_ENTRY* dir_ent) {

  int32_t rc = -1;

  uint8_t* e = NULL;

  for (int16_t i = 0; i < FAT12_NUM_ROOT_DIR_ENTRIES; i++) {
    uint8_t p = fat->root_dir[ 32 * i ];
    if (p == 0x00 || p == 0xe5) {
      e = fat->root_dir + 32 * i;
      break;
    }
  }

  if (e != NULL) {
    memcpy(e, dir_ent->name, 11);
    if (e[0] == 0xe5) e[0] = (uint8_t)0x05;
    e[11] = dir_ent->attr;
    memcpy(e + 12, dir_ent->name2, 10);
    e[22] = dir_ent->wrt_time & 0xff;
    e[23] = (dir_ent->wrt_time >> 8) & 0xff;
    e[24] = dir_ent->wrt_date & 0xff;
    e[25] = (dir_ent->wrt_date >> 8) & 0xff;
    e[26] = dir_ent->fst_clus_lo & 0xff;
    e[27] = (dir_ent->fst_clus_lo >> 8) & 0xff;
    e[28] = dir_ent->file_size & 0xff;
    e[29] = (dir_ent->file_size >> 8) & 0xff;
    e[30] = (dir_ent->file_size >> 16) & 0xff;
    e[31] = (dir_ent->file_size >> 24) & 0xff;
    rc = 0;
  }

  return rc;
}

int32_t fat12_add_dir_entry(FAT12* fat, int16_t current_dir_cluster, FAT12_DIR_ENTRY* dir_ent) {

  int32_t rc = -1;

  uint8_t* e = NULL;

  static uint8_t dir_table[ XDF_SECTOR_BYTES ];

  do {

    fat12_read_cluster(fat, current_dir_cluster, dir_table);

    for (int16_t i = 0; i < 32; i++) {
      uint8_t p = dir_table[ 32 * i ];
      if (p == 0x00 || p == 0xe5) {
        e = dir_table + 32 * i;
        break;
      }
    }

    if (e != NULL) {
      memcpy(e, dir_ent->name, 11);
      if (e[0] == 0xe5) e[0] = (uint8_t)0x05;
      e[11] = dir_ent->attr;
      memcpy(e + 12, dir_ent->name2, 10);
      e[22] = dir_ent->wrt_time & 0xff;
      e[23] = (dir_ent->wrt_time >> 8) & 0xff;
      e[24] = dir_ent->wrt_date & 0xff;
      e[25] = (dir_ent->wrt_date >> 8) & 0xff;
      e[26] = dir_ent->fst_clus_lo & 0xff;
      e[27] = (dir_ent->fst_clus_lo >> 8) & 0xff;
      e[28] = dir_ent->file_size & 0xff;
      e[29] = (dir_ent->file_size >> 8) & 0xff;
      e[30] = (dir_ent->file_size >> 16) & 0xff;
      e[31] = (dir_ent->file_size >> 24) & 0xff;

      fat12_write_cluster(fat, current_dir_cluster, dir_table);

      rc = 0;

      break;
    }

    int16_t next = fat12_get_allocation(fat, current_dir_cluster);
    if (next == FAT12_EOC) break;

    current_dir_cluster = next;

  } while (e == NULL);

  return rc;
}

int32_t fat12_read_cluster(FAT12* fat, int16_t cluster, uint8_t* buf) {
  return xdf_read(fat->xdf, 11 + cluster - 2, 1, buf);
}

int32_t fat12_write_cluster(FAT12* fat, int16_t cluster, uint8_t* buf) {
//  printf("write cluster %d\n",cluster);
  return xdf_write(fat->xdf, 11 + cluster - 2, 1, buf);
}
