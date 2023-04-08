#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <doslib.h>
#include <iocslib.h>
#include "fat12.h"
#include "xdf.h"
#include "xdfarc.h"

//
//  add directory recursively
//
static int32_t add_dir(FAT12* fat, uint8_t* dir_name, FAT12_DIR_ENTRY* current_dir_ent, int16_t sort_entries, size_t* resume_marker) {

  // default return code
  int32_t rc = -1;

  // file read handle
  FILE* fp = NULL;

  // allocated cluster ID array
  static int16_t clusters[ FAT12_NUM_ALLOCATIONS ];

  // file data read buffer
  static uint8_t read_buf[ XDF_SECTOR_BYTES ];




  // dir name + wild card for directory scan
  uint8_t wild_name[ MAX_PATH_LEN ];
  strcpy(wild_name, dir_name);
  uint8_t wild_name_suffix = strlen(wild_name) > 0 ? wild_name[ strlen(wild_name) - 1 ] : '\0';
  if (wild_name_suffix != '\0' && wild_name_suffix != ':' && wild_name_suffix != '\\' && wild_name_suffix != '/') {
    strcat(wild_name, "\\*.*");
  } else {
    strcat(wild_name, "*.*");
  }

  // scan directory
  struct FILBUF filbuf = { 0 };
  if (FILES(&filbuf, wild_name, 0x31) < 0) {    // read only + file + dir
    if (current_dir_ent == NULL) {
      printf("error: found no file. (%s)\n", wild_name);    // show message at root dir only
    }
    goto exit;
  }

  do {

    // full path name
    uint8_t path_name [ MAX_PATH_LEN ];
    strcpy(path_name, wild_name);
    path_name[ strlen(path_name) - 3 ] = '\0';
    strcat(path_name, filbuf.name);                 // replace last *.* with real file name

    // parent/sub dir entry for dir
    FAT12_DIR_ENTRY parent_dir_ent = { 0 };
    FAT12_DIR_ENTRY sub_dir_ent = { 0 };

    if (filbuf.atr & FAT12_DIR_ATTR_DIRECTORY) {

      // count files in sub dir
      struct FILBUF sub_buf = { 0 };
      uint8_t sub_dir_name [ MAX_PATH_LEN ];
      strcpy(sub_dir_name, path_name);
      strcat(sub_dir_name, "\\*.*");
      int16_t num_sub_files = 2;
      if (FILES(&sub_buf, sub_dir_name, 0x31) >= 0) {
        do {
          if (sub_buf.filelen < XDF_SIZE_BYTES) {
            num_sub_files++;
          }
        } while (NFILES(&sub_buf) >= 0);
      }

      // allocate clusters
      int16_t num_clusters = 1 + num_sub_files / 32;        // 1 cluster (1024 bytes can support 32 bytes * 32 entries)
      int16_t found = fat12_find_free_clusters(fat, num_clusters, clusters);
      if (found < num_clusters) {
        printf("%-24s ... disk full!!\n", path_name);
        goto next;        
      }
      for (int16_t i = 0; i < num_clusters; i++) {
        memset(read_buf, 0, XDF_SECTOR_BYTES);
        if (fat12_write_cluster(fat, clusters[i], read_buf) != 0) {
          printf("%-24s ... file write error!!\n", path_name);
          goto next;
        }
        fat12_set_allocation(fat, clusters[i], i < ( num_clusters - 1 ) ? clusters[ i + 1 ] : FAT12_EOC);
      }
      filbuf.filelen = 0;

    } else {

      // file
      int16_t num_clusters = 1 + filbuf.filelen / XDF_SECTOR_BYTES;
      if (num_clusters > FAT12_NUM_ALLOCATIONS) {
        // too large to archive
        printf("%-24s ... skipped (%6d bytes)\n", path_name, filbuf.filelen);
        goto next;
      }
      int16_t found = fat12_find_free_clusters(fat, num_clusters, clusters);
      if (found < num_clusters) {
        printf("%-24s ... skipped (%6d bytes)\n", path_name, filbuf.filelen);
        goto next;
      }

      FILE* fp = fopen(path_name, "rb");
      if (fp == NULL) {
        printf("%-24s ... file open error!!\n", path_name);
        goto next;
      }
      for (int16_t i = 0; i < num_clusters; i++) {
        memset(read_buf, 0, XDF_SECTOR_BYTES);
        if (fread(read_buf, sizeof(uint8_t), XDF_SECTOR_BYTES, fp) == 0) {
          printf("%-24s ... file read error!!\n", path_name);
          goto next;
        }
        if (fat12_write_cluster(fat, clusters[i], read_buf) != 0) {
          printf("%-24s ... file write error!!\n", path_name);
          goto next;
        }
        fat12_set_allocation(fat, clusters[i], i < ( num_clusters - 1 ) ? clusters[ i + 1 ] : FAT12_EOC);
      }
      fclose(fp);
      fp = NULL;

    }

    FAT12_DIR_ENTRY ent = { 0 };
    fat12_create_dir_entry(fat, &ent, filbuf.name, filbuf.atr, filbuf.date, filbuf.time, filbuf.filelen, clusters[0]);
    if (current_dir_ent == NULL) {
      if (fat12_add_root_dir_entry(fat, &ent) != 0) {
        printf("error: too many files in the root directory.\n");
        goto exit;
      }
    } else {
      if (fat12_add_dir_entry(fat, current_dir_ent, &ent) != 0) {
        printf("error: too many files in the root directory.\n");
        goto exit;
      }
    }
    
    if (filbuf.atr & FAT12_DIR_ATTR_DIRECTORY) {
      printf("%-24s ... added (directory)\n", path_name);
      add_dir(fat, path_name, &sub_dir_ent, sort_entries, resume_marker);
    } else {
      printf("%-24s ... added (%6d bytes)\n", path_name, filbuf.filelen);        
    }

next:
    if (fp != NULL) {
      fclose(fp);
      fp = NULL;
    }

  } while (NFILES(&filbuf) >= 0);

exit:
  return rc;
}

//
//  show help message
//
static void show_help_message() {
  printf("usage: xdfarc <xdf-name> <dir-name>\n");
  printf("options:\n");
//  printf("   -m ... create multiple XDF automatically\n");
//  printf("   -s ... sort directory entries\n");
  printf("   -h ... show help message\n");
}

//
//  main
//
int32_t main(int32_t argc, uint8_t* argv[]) {

  // default return code
  int32_t rc = -1;

  // xdf name
  uint8_t* xdf_name = NULL;

  // source dir name
  uint8_t* dir_name = NULL;

  // XDF handle
  static XDF xdf = { 0 };

  // FAT handle
  static FAT12 fat = { 0 };

  // create multiple XDFs
  int16_t multi_xdf = 0;

  // sort directory entries
  int16_t sort_entries = 0;

  // resume point marker
  size_t resume_marker = 0;

  // xdf index
  int32_t xdf_index = 1;

  // credit
  printf("XDFARC.X - XDF archiver for X680x0 " VERSION " by tantan\n");

  // check command line
  if (argc < 2) {
    show_help_message();
    goto exit;
  }

  // parse command lines
  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'h') {
        show_help_message();
        goto exit;
      } else if (argv[i][1] == 'm') {
        multi_xdf = 1;
      } else if (argv[i][1] == 's') {
        sort_entries = 1;
      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      if (xdf_name == NULL) {
        xdf_name = argv[i];
      } else if (dir_name == NULL) {
        dir_name = argv[i];
      } else {
        printf("error: too many arguments.\n");
        goto exit;
      }
    }
  }

  if (xdf_name == NULL) {
    show_help_message();
    goto exit;
  }

loop:
  // format XDF
  printf("XDF name: %s\n", xdf_name);
  if (xdf_init(&xdf, xdf_name) != 0) {
    goto exit;
  }

  // initialize FAT
  if (fat12_init(&fat, &xdf) != 0) {
    goto exit;
  }

  // add files and dirs
  rc = add_dir(&fat, (dir_name == NULL || strcmp(dir_name, ".") == 0) ? (uint8_t*)"" : dir_name, NULL, sort_entries, &resume_marker);

  if (rc == 0) {
    printf("done.\n");
    goto exit;
  }

  if (rc == 1 && multi_xdf) {     // disk full and allow multi XDF

    // close current FAT
    fat12_close(&fat);

    // close current XDF
    xdf_close(&xdf);

    // next XDF name
    uint8_t next_xdf_name [ MAX_PATH_LEN ];
    strcpy(next_xdf_name, xdf_name);
    uint8_t* next_xdf_ext = next_xdf_name + strlen(next_xdf_name) - 4;
    sprintf(next_xdf_ext, "%d%s", ++xdf_index, xdf_name + strlen(xdf_name) - 4);
    strcpy(xdf_name, next_xdf_name);
    
    goto loop;
  }

exit:

  // close FAT
  fat12_close(&fat);

  // close XDF
  xdf_close(&xdf);

  return rc;
}