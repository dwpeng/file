#ifndef __DWP_FILE_H__
#define __DWP_FILE_H__

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_BUFF_SIZE 1024 * 512 // 512KB

#define LF '\r'
#define CR '\n'
#define LFCR "\r\n"

typedef enum LineBreakType {
  LINE_BREAK_LF = 0, /* mac os */
  LINE_BREAK_CR,     /* unix */
  LINE_BREAK_LFCR,   /* windows */
  LINE_BREAK_UNKNOWN
} LineBreakType;

typedef struct XFile XFile;
struct XFile {
  char *path;
  char *mode;
  LineBreakType line_break;
  int open;
  void *fp;
  long int offset;
  size_t length;
};

typedef struct XFileHandle {
  void *(*open)(XFile *file);
  size_t (*read)(XFile *file, size_t size, char *buff);
  size_t (*write)(XFile *file, size_t size, char *buff);
  size_t (*write_format)(XFile *file, char *format, ...);
  int (*seek)(XFile *file, long int offset, int whence);
  int (*close)(XFile *file);
  size_t (*length)(XFile *file);
  char *(*readline)(XFile *file);
} XFileHandle;

#define FP(file) (file->open ? file->fp : NULL)

XFile *xfile(char *path, char *mode);
void destory_xfile(XFile *file);

extern XFileHandle plainFile;

#endif
