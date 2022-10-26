#ifndef __DWP_FILE_H__
#define __DWP_FILE_H__

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_BUFF_SIZE 1024 * 512       // 512KB
#define FILE_BUFF_SIZE 1024 * 512      // 512KB

#define LF '\r'
#define CR '\n'
#define LFCR "\r\n"

#ifdef _WIN32
#define DEFAULT_BREAK_LINE LFCR
#elif __linux__
#define DEFAULT_BREAK_LINE ((char *)LF)
#elif __APPLE__
#define DEFAULT_BREAK_LINE ((char *)LF)
#else
#define DEFAULT_BREAK_LINE ((char *)CR)
#endif

typedef enum LineBreakType {
  LINE_BREAK_LF = 0, /* mac os/unix */
  LINE_BREAK_CR,     /* unix */
  LINE_BREAK_LFCR,   /* windows */
  LINE_BREAK_UNKNOWN
} LineBreakType;

typedef struct XFile XFile;

struct XFile {
  /* 文件路径 */
  char *path;

  /* 文件打开模式 */
  /**
   * [0][1][2][3][4][5][6][7]
   * [#][r][w][+][b][w][a][r]
   */

  char mode;

  /* 打开模式的字符串模式 */
  char *mode_str;
  /* 文件换行符 */
  LineBreakType line_break;
  /* 文件是否处于打开状态 */
  int open;
  /* 文件是否是空文件 */
  int blank;
  /* 文件句柄 */
  void *fp;
  /* 文件读取/写入的位置 */
  long int offset;
  /* 文件长度，按字节计 */
  size_t length;
  struct {
    /* buff缓存 */
    char *data;
    /* buff中的数据偏移 */
    size_t offset;
    /* buff的大小 */
    int buff_size;
    /* 文件是否读取完成 */
    int eof;
  } buff[2];
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
