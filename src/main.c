#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <doslib.h>
#include <iocslib.h>
#include "xdf.h"
#include "xdfarc.h"

//
//  show help message
//
static void show_help_message() {
  printf("usage: xdfarc <xdf-prefix(max6bytes)> <source-dir>\n");
  printf("options:\n");
  printf("   -h ... show help message\n");
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

  // xdf prefix name
  uint8_t* xdf_prefix = NULL;

  // src file pointer
  FILE* fp = NULL;

  // file split pointers
  FILE_SPLIT* file_splits = NULL;

  // XDF handlers
  static XDF xdf[ XDF_MAX_DRIVES ];
  memset(xdf, 0, sizeof(XDF) * XDF_MAX_DRIVES);

  // num of XDF
  int16_t num_xdf = 0;

  // credit
  printf("XDFARC.X - XDF format file archiver for X680x0 " VERSION " by tantan\n");

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
      } else {
        printf("error: unknown option (%s).\n",argv[i]);
        goto exit;
      }
    } else {
      if (xdf_prefix == NULL) {
        xdf_prefix = argv[i];
        if (strlen(xdf_prefix) < 1 || strlen(xdf_prefix) > 6) {
          show_help_message();
          goto exit;
        }
      } else if (src_dir == NULL) {
        src_dir = argv[i];
      } else {
        printf("error: too many arguments.\n");
        goto exit;
      }
    }
  }

  if (xdf_prefix == NULL || src_dir == NULL) {
    show_help_message();
    goto exit;
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
    if (filbuf.name[0] != '.' && !(strlen(filbuf.name) >= 5 && stricmp(filbuf.name + strlen(filbuf.name) - 4, ".xdf") == 0)) {
      file_split_count += ( 1 + filbuf.filelen / FD_2HD_BYTES );
      total_bytes += filbuf.filelen;
    }
    code = NFILES(&filbuf);
  } while (code >= 0);

  if (total_bytes >= FD_2HD_BYTES * XDF_MAX_DRIVES) {
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

  if (filbuf.name[0] != '.' && !(strlen(filbuf.name) >= 5 && stricmp(filbuf.name + strlen(filbuf.name) - 4, ".xdf") == 0)) {
    if (filbuf.filelen > FD_2HD_BYTES) {
      int32_t len = filbuf.filelen;
      int16_t chunk = 1;
      while (len > 0) {
        FILE_SPLIT* fs = &(file_splits[spi++]);
        fs->chunk = chunk++;
        fs->num_chunks = 1 + filbuf.filelen / FD_2HD_BYTES;
        strcpy(fs->name, filbuf.name);
        strcpy(fs->path_name, src_dir);
        strcat(fs->path_name, "\\");
        strcat(fs->path_name, fs->name);
        strcat(fs->out_name, fs->name);
        uint8_t* file_ext = strchr(fs->out_name, '.');
        if (file_ext == NULL) {
          file_ext = fs->out_name + strlen(fs->out_name);
        }
        sprintf(file_ext, ".%03d", fs->chunk - 1);        
        fs->size_bytes = len > FD_2HD_BYTES ? FD_2HD_BYTES : len;
        len -= FD_2HD_BYTES;
        if (chunk > 10) {
          printf("error: too large file. (%s)\n", filbuf.name);
          goto exit;
        }
      }
    } else {
      FILE_SPLIT* fs = &(file_splits[spi++]);
      strcpy(fs->name, filbuf.name);
      strcpy(fs->path_name, src_dir);
      strcat(fs->path_name, "\\");
      strcat(fs->path_name, fs->name);
      strcat(fs->out_name, fs->name);
      fs->size_bytes = filbuf.filelen;
    }
  }
  for (int16_t i = 1; i < file_count; i++) {
    NFILES(&filbuf);
    if (filbuf.name[0] == '.') continue;
    if (strlen(filbuf.name) >= 5 && stricmp(filbuf.name + strlen(filbuf.name) - 4, ".xdf") == 0) continue;
    if (filbuf.filelen > FD_2HD_BYTES) {
      int32_t len = filbuf.filelen;
      int16_t chunk = 1;
      while (len > 0) {
        FILE_SPLIT* fs = &(file_splits[spi++]);
        fs->chunk = chunk++;
        fs->num_chunks = 1 + filbuf.filelen / FD_2HD_BYTES;
        strcpy(fs->name, filbuf.name);
        strcpy(fs->path_name, src_dir);
        strcat(fs->path_name, "\\");
        strcat(fs->path_name, fs->name);
        strcat(fs->out_name, fs->name);
        uint8_t* file_ext = strchr(fs->out_name, '.');
        if (file_ext == NULL) {
          file_ext = fs->out_name + strlen(fs->out_name);
        }
        sprintf(file_ext, ".%03d", fs->chunk - 1);
        fs->size_bytes = len > FD_2HD_BYTES ? FD_2HD_BYTES : len;
        len -= FD_2HD_BYTES;
        if (chunk > 10) {
          printf("error: too large file. (%s)\n", filbuf.name);
          goto exit;
        }
      }
    } else {
      FILE_SPLIT* fs = &(file_splits[spi++]);
      strcpy(fs->name, filbuf.name);
      strcpy(fs->path_name, src_dir);
      strcat(fs->path_name, "\\");
      strcat(fs->path_name, fs->name);
      strcat(fs->out_name, fs->name);
      fs->size_bytes = filbuf.filelen;
    }
  }

  // sort by file size
  qsort(file_splits, file_split_count, sizeof(FILE_SPLIT), (int(*)(const void*, const void*))&compare_file_split);

  // allocation
  size_t xdf_sizes[ XDF_MAX_DRIVES ];
  int16_t overflow;
  num_xdf = 1 + total_bytes / FD_2HD_BYTES;
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
        if (num_xdf > XDF_MAX_DRIVES) {
          printf("error: XDF allocation failure.\n");
          goto exit;
        }
        break;
      }
    }
  } while (overflow); 

  // create target XDF images
  int16_t overwrite = 0;
  for (int16_t i = 0; i < num_xdf; i++) {
    static uint8_t xdf_path[ MAX_PATH_LEN ];
    sprintf(xdf_path, "%s%02d.xdf", xdf_prefix, i);
    if (FILES(&filbuf, xdf_path, 0x20) >= 0) {
      if (!overwrite) {
        printf("warning: output XDF image (%s) already exists. overwrite? (y/n)", xdf_path);
        uint8_t c;
        do {
          c = INKEY();
          if (c == 'n' || c == 'N') {
            printf("\ncanceled.\n");
            goto exit;
          }
        } while (c != 'y' && c != 'Y');
        printf("\n");
        overwrite = 1;
      }
    }
    if (xdf_init(&(xdf[i]), xdf_path, i) != 0)  {
      goto exit;
    }
    printf("Created and formatted %s\n", xdf_path);
  }
/*
  // distributed copy
  static uint8_t fread_buf[ FREAD_BUF_BYTES ];
  static uint8_t out_path_name[ MAX_PATH_LEN ];
  for (int16_t i = 0; i < file_split_count; i++) {
    FILE_SPLIT* fs = &(file_splits[i]);
    printf("%s%02d <-- %s (%d bytes)\n", base_name, fs->destination, fs->out_name, fs->size_bytes);
    if (fs->chunk > 0) {
      fp = fopen(fs->path_name, "rb");
      if (fp == NULL) {
        printf("error: file read open error. (%s)\n", fs->path_name);
        goto exit;
      }
      sprintf(out_path_name, "%s\\%s%02d\\%s", out_dir, base_name, fs->destination, fs->out_name);
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
      sprintf(out_path_name, "%s\\%s%02d\\%s", out_dir, base_name, fs->destination, fs->out_name);
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
  for (int16_t i = 0; i < file_split_count; i++) {
    FILE_SPLIT* fs = &(file_splits[i]);
    if (fs->chunk == 0 || fs->chunk < fs->num_chunks) continue;
    if (fo == NULL) {
      size_t min_size = xdf_sizes[0];
      int16_t min_xdf = 0;
      for (int16_t j = 1; j < num_xdf; j++) {
        if (xdf_sizes[j] < min_size) {
          min_size = xdf_sizes[j];
          min_xdf = j;
        }
      }
      static uint8_t out_path_name[ MAX_PATH_LEN ];
      sprintf(out_path_name, "%s\\%s%02d\\%s", out_dir, base_name, min_xdf, "zsplit_merge.bat");
      fo = fopen(out_path_name, "w");
      if (fo == NULL) {
        printf("error: file write open error. (%s)\n", out_path_name);
        goto exit;
      }
    }
    static uint8_t line[ MAX_PATH_LEN * 12 ];
    strcpy(line, "copy ");
    for (int16_t j = 0; j < fs->num_chunks; j++) {
      if (j > 0) strcat(line, "+");
      strcat(line, fs->out_name);
      sprintf(line + strlen(line) - 3, "%03d", j);
    }
    strcat(line, " ");
    strcat(line, fs->name);
    strcat(line, "\n");
    fputs(line, fo);
  }
*/
exit:

  for (int16_t i = 0; i < num_xdf; i++) {
    xdf_close(&(xdf[i]));
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