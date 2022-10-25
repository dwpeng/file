#include "file.h"

void *plain_open(XFile *file) {
  FILE *fp;
  if ((fp = fopen(file->path, file->mode)) == NULL) {
    return NULL;
  }
  file->fp = fp;
  file->open = 1;
  file->line_break = LINE_BREAK_UNKNOWN;
  return fp;
}

size_t plain_read(XFile *file, size_t size, char *buff) {
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  return fread(buff, sizeof(char), size, fp);
}

size_t plain_write(XFile *file, size_t size, char *buff) {
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  return fwrite(buff, sizeof(char), size, fp);
}

size_t plain_write_format(XFile *file, char *format, ...) {
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }

  // TODO
  char buff[1024] = {0};
  va_list valist;
  va_start(valist, format);
  vsnprintf(buff, sizeof(buff), format, valist);
  va_end(valist);
  return plain_write(fp, strlen(buff), buff);
}

int plain_close(XFile *file) {
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  return fclose(fp);
}

int plain_seek(XFile *file, long int offset, int whence) {
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  fseek(fp, offset, whence);
}

size_t plain_length(XFile *file) {
  plain_seek(file, 0, SEEK_END);
  size_t length = ftell(file->fp);
  plain_seek(file, file->offset, SEEK_SET);
  return length;
}

char *plain_readline(XFile *file) {
  void *fp = FP(file);
  if (fp == NULL) {
    return NULL;
  }

  if (file->length && ftell(fp) == file->length) {
    return NULL;
  }

  char c;
  // 检查当前文件是以什么方式结尾的
  if (file->line_break == LINE_BREAK_UNKNOWN) {
    file->length = plain_length(file);
    if (!file->length) {
      return NULL;
    }
    while (1) {
      c = fgetc(fp);
      if (!c)
        break;
      if (c == LF || c == CR) {
        char next = fgetc(fp);
        if (!next)
          break;
        else if (next == CR) {
          file->line_break = LINE_BREAK_LFCR;
        } else if (c == LF) {
          file->line_break = LINE_BREAK_LF;
        } else {
          file->line_break = LINE_BREAK_CR;
        }
        plain_seek(file, 0, SEEK_SET);
        break;
      }
    }
  }

  char *s = malloc(MIN_BUFF_SIZE * sizeof(char) + 1);
  size_t count = 0;
  char line_break = file->line_break == LINE_BREAK_CR ? CR : LF;
  int next = file->line_break == LINE_BREAK_LFCR ? 1 : 0;

  while (1) {
    c = fgetc(fp);
    if (!c || c == line_break) {
      if (c && next) {
        fgetc(fp);
      }
      break;
    }
    if (count == MIN_BUFF_SIZE) {
      s = realloc(s, sizeof((int)((float)MIN_BUFF_SIZE * 1.5)));
    }
    s[count] = c;
    count++;
  }

  file->offset = ftell(file->fp);
  s[count] = '\0';
  return s;
}

XFileHandle plainFile = {
    .close = plain_close,
    .open = plain_open,
    .read = plain_read,
    .readline = plain_readline,
    .write = plain_write,
    .write_format = plain_write_format,
    .seek = plain_seek,
    .length = plain_length,
};

XFile *xfile(char *path, char *mode) {
  XFile *file = malloc(sizeof(XFile));
  file->path = path;
  file->mode = mode;
  file->length = 0;
  file->offset = 0;
  return file;
}

void destory_xfile(XFile *file) {
  if (file) {
    free(file);
  }
}
