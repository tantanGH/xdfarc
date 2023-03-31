#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <doslib.h>
#include <iocslib.h>
#include "ezsplit.h"

//
//  show help message
//
static void show_help_message() {
  printf("usage: ezsplit [options] <source-dir>\n");
  printf("options:\n");
  printf("   -o <output-dir> ... output directory\n");
  printf("   -h              ... show help message\n");
}

//
//  quick sort helper
//
static int32_t compare_file_split(const void* n1, const void* n2) {
  FILE_SPLIT fs1 = *((FILE_SPLIT*)n1);
  FILE_SPLIT fs2 = *((FILE_SPLIT*)n2);
  return fs1.size_bytes > fs2.size_bytes ? -1 :
         fs1.size_bytes < fs2.size_bytes ?  1 : strcmp(fs1.path_name, fs2.path_name);
}

//
//  main
//
int32_t main(int32_t argc, uint8_t* argv[]) {

  // default return code
  int32_t rc = -1;

  // src dir name
  uint8_t* src_dir = NULL;

  // out dir name
  uint8_t* out_dir = NULL;

  // src file pointer
  FILE* fp = NULL;

  // out file pointer
  FILE* fo = NULL;

  // file split pointers
  FILE_SPLIT* file_splits = NULL;

  // credit
  printf("EZSPLIT.X - A simple file set split utility for multiple XDFs " VERSION " by tantan\n");

  // check command line
  if (argc < 2) {
    show_help_message();
    goto exit;
  }

  // parse command lines
  for (int16_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
      if (argv[i][1] == 'o') {
        if (i + 1 < argc) {
          out_dir = argv[ i + 1 ];
        } else {
          show_help_message();
          goto exit;
        }
      } else if (argv[i][1] == 'h') {
        show_help_message();
        goto exit;
      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      if (src_dir != NULL) {
        printf("error: too many files.\n");
        goto exit;
      }
      src_dir = argv[i];
    }
  }

  if (src_dir == NULL) {
    show_help_message();
    goto exit;
  }

  if (out_dir == NULL) {
    out_dir = src_dir;
  }

  // scan path 1: count candidate files
  struct FILBUF filbuf = { 0 };
  uint8_t scan_path[ MAX_PATH_LEN ];
  strcpy(scan_path, src_dir);
  strcat(scan_path, "\\*.*");

  int32_t code = FILES(&filbuf, scan_path, 0x20);
  if (code < 0) {
    printf("error: no file is found in the source dir.\n");
    goto exit;
  }

  size_t file_count = 0;
  size_t file_split_count = 0;
  size_t total_bytes = 0;
  do {
    file_count++;
    if (filbuf.name[0] != '.') {
      file_split_count += ( 1 + filbuf.filelen / FD_2HD_BYTES );
      total_bytes += filbuf.filelen;
    }
    code = NFILES(&filbuf);
  } while (code >= 0);

  if (total_bytes >= FD_2HD_BYTES * MAX_XDF) {
    printf("error: total size is too large.\n");
    goto exit;
  }

  // scan path 2: obtain candidate file attributes
  file_splits = (FILE_SPLIT*)MALLOC( sizeof(FILE_SPLIT) * file_split_count );
  int16_t spi = 0;
  memset(file_splits, 0, sizeof(FILE_SPLIT) * file_split_count );
  code = FILES(&filbuf, scan_path, 0x20);
  if (code < 0) {
    printf("error: no file is found in the source dir.\n");
    goto exit;
  }

  if (filbuf.name[0] != '.') {
    if (filbuf.filelen > FD_2HD_BYTES) {
      int32_t len = filbuf.filelen;
      int16_t chunk = 1;
      while (len > 0) {
        FILE_SPLIT* fs = &(file_splits[spi++]);
        fs->chunk = chunk++;
        strcpy(fs->path_name, src_dir);
        strcat(fs->path_name, "\\");
        strcat(fs->path_name, filbuf.name);
        strcat(fs->out_name, filbuf.name);
        uint8_t* file_ext = strchr(fs->out_name, '.');
        if (file_ext == NULL) {
          file_ext = fs->out_name + strlen(fs->out_name);
        }
        sprintf(file_ext, ".%03d", fs->chunk - 1);        
        fs->size_bytes = len > FD_2HD_BYTES ? FD_2HD_BYTES : len;
        len -= FD_2HD_BYTES;
      }
    } else {
      FILE_SPLIT* fs = &(file_splits[spi++]);
      strcpy(fs->path_name, src_dir);
      strcat(fs->path_name, "\\");
      strcat(fs->path_name, filbuf.name);
      strcat(fs->out_name, filbuf.name);
      fs->size_bytes = filbuf.filelen;
    }
  }
  for (int16_t i = 1; i < file_count; i++) {
    NFILES(&filbuf);
    if (filbuf.name[0] == '.') continue;
    if (filbuf.filelen > FD_2HD_BYTES) {
      int32_t len = filbuf.filelen;
      int16_t chunk = 1;
      while (len > 0) {
        FILE_SPLIT* fs = &(file_splits[spi++]);
        fs->chunk = chunk++;
        strcpy(fs->path_name, src_dir);
        strcat(fs->path_name, "\\");
        strcat(fs->path_name, filbuf.name);
        strcat(fs->out_name, filbuf.name);
        uint8_t* file_ext = strchr(fs->out_name, '.');
        if (file_ext == NULL) {
          file_ext = fs->out_name + strlen(fs->out_name);
        }
        sprintf(file_ext, ".%03d", fs->chunk - 1);
        fs->size_bytes = len > FD_2HD_BYTES ? FD_2HD_BYTES : len;
        len -= FD_2HD_BYTES;
      }
    } else {
      FILE_SPLIT* fs = &(file_splits[spi++]);
      strcpy(fs->path_name, src_dir);
      strcat(fs->path_name, "\\");
      strcat(fs->path_name, filbuf.name);
      strcat(fs->out_name, filbuf.name);
      fs->size_bytes = filbuf.filelen;
    }
  }

  // sort by file size
  qsort(file_splits, file_split_count, sizeof(FILE_SPLIT), (int(*)(const void*, const void*))&compare_file_split);

  // allocation
  size_t xdf_sizes[ MAX_XDF ];
  size_t num_xdf = 1 + total_bytes / FD_2HD_BYTES;
  int16_t overflow;
  do {
    overflow = 0;
    memset(xdf_sizes, 0, sizeof(size_t) * num_xdf);
    for (int16_t i = 0; i < file_split_count; i++) {
      size_t min_size = xdf_sizes[0];
      int16_t min_xdf = 0;
      for (int16_t j = 1; j < num_xdf; j++) {
        if (xdf_sizes[j] < min_size) {
          min_size = xdf_sizes[j];
          min_xdf = j;
        }
      }
      file_splits[i].destination = min_xdf;
      xdf_sizes[ min_xdf ] += file_splits[i].size_bytes;
      if (xdf_sizes[ min_xdf ] > FD_2HD_BYTES) {
        overflow = 1;
        num_xdf++;
        break;
      }
    }
  } while (overflow); 

  // create target directories
  for (int16_t i = 0; i < num_xdf; i++) {
    static uint8_t xdf_dir[ MAX_PATH_LEN ];
    sprintf(xdf_dir, "%s\\xdf%02d", out_dir, i);
    if (MKDIR(xdf_dir) < 0) {
      printf("error: xdf directory creation error.\n");
      goto exit;
    }
  }

  // distributed copy
  static uint8_t fread_buf[ FREAD_BUF_BYTES ];
  static uint8_t out_path_name[ MAX_PATH_LEN ];
  for (int16_t i = 0; i < file_split_count; i++) {
    FILE_SPLIT* fs = &(file_splits[i]);
    printf("xdf%02d <-- %s (%d bytes)\n", fs->destination, fs->out_name, fs->size_bytes);
    if (fs->chunk > 0) {
      fp = fopen(fs->path_name, "rb");
      if (fp == NULL) {
        printf("error: file read open error. (%s)\n", fs->path_name);
        goto exit;
      }
      sprintf(out_path_name, "%s\\xdf%02d\\%s", out_dir, fs->destination, fs->out_name);
      fo = fopen(out_path_name, "wb");
      if (fo == NULL) {
        printf("error: file write open error. (%s)\n", out_path_name);
        goto exit;
      }
      fseek(fp, FD_2HD_BYTES * (fs->chunk - 1), SEEK_SET);
      size_t write_len = 0;
      do {
        size_t len = fread(fread_buf, 1, FREAD_BUF_BYTES, fp);
        if (len == 0) break;
        size_t len2 = fwrite(fread_buf, 1, (fs->size_bytes - write_len) < len ? fs->size_bytes - write_len : len, fo);
        write_len += len2;
      } while (write_len < fs->size_bytes);
      fclose(fo);
      fo = NULL;
      fclose(fp);
      fp = NULL;
    } else {
      fp = fopen(fs->path_name, "rb");
      if (fp == NULL) {
        printf("error: file read open error. (%s)\n", fs->path_name);
        goto exit;
      }
      sprintf(out_path_name, "%s\\xdf%02d\\%s", out_dir, fs->destination, fs->out_name);
      fo = fopen(out_path_name, "wb");
      if (fo == NULL) {
        printf("error: file write open error. (%s)\n", out_path_name);
        goto exit;
      }
      size_t write_len = 0;
      do {
        size_t len = fread(fread_buf, 1, FREAD_BUF_BYTES, fp);
        if (len == 0) break;
        size_t len2 = fwrite(fread_buf, 1, (fs->size_bytes - write_len) < len ? fs->size_bytes - write_len : len, fo);
        write_len += len2;
      } while (write_len < fs->size_bytes);
      fclose(fo);
      fo = NULL;
      fclose(fp);
      fp = NULL;
    }
  }

  // merge batch


exit:

  if (fo != NULL) {
    fclose(fo);
    fo = NULL;
  }

  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  if (file_splits != NULL) {
    MFREE((uint32_t)file_splits);
    file_splits = NULL;
  }

  return rc;
}