#include "file.h"

size_t plain_length(XFile *file);
int plain_seek(XFile *file, long int offset, int whence);
void plain_breakline(XFile *file);

#define set1(mode, n) (mode) |= (0x1 << n)
#define if1(mode, n) ((((mode) & (0x1 << n))) == (0x1 << n))

#define Writeable(mode) (if1(mode, 5) || if1(mode, 3) || if1(mode, 2))
#define Readable(mode) (if1(mode, 1) || if1(mode, 3) || if1(mode, 7))

#define Read(mode)                                                             \
  set1(mode, 7);                                                               \
  set1(mode, 1)
#define Write(mode)                                                            \
  set1(mode, 2);                                                               \
  set1(mode, 5)
#define ReadWrite(mode)                                                        \
  Read(mode);                                                                  \
  Write(mode)
#define Append(mode)                                                           \
  set1(mode, 5);                                                               \
  set1(mode, 2)
#define Binary(mode) set1(mode, 4);

/**
 * @brief 检查文件的换行符
 *
 * @param file
 */
void plain_breakline(XFile *file) {
  /* 检查文件是否可写 */
  if (!Readable(file->mode)) {
    return;
  }

  /* 文件为空直接退出 */
  if (file->blank) {
    return;
  }

  /* 获取文件句柄 */
  FILE *fp = FP(file);

  char c;
  if (file->line_break == LINE_BREAK_UNKNOWN) {
    /* 在文件的换行符未知的情况下在进行检查 */
    while (1) {
      /* 从缓存中拿出一个字符 */
      c = file->buff[0].data[file->buff[0].offset];
      file->buff[0].offset++;
      /* 如果文件长度大于一，且检测到了LF，那么继续检查是否存在CR */
      if (c == LF && file->length > 1) {
        char next = file->buff[0].data[file->buff[0].offset];
        file->buff[0].offset++;
        if (next == CR) {
          file->line_break = LINE_BREAK_LFCR;
          goto reset;
        }
      }

      /* 只有单字符 */
      if (c == LF) {
        file->line_break = LINE_BREAK_LF;
      } else {
        file->line_break = LINE_BREAK_CR;
      }
      goto reset;
    }
  }

reset:
  plain_seek(file, 0, SEEK_SET);
  file->buff[0].offset = 0;
  return;
}

/**
 * @brief 自动从文件中读取内容，并更新缓存
 *
 * @param file
 * @return size_t
 */
size_t plain_auto_read(XFile *file) {
  /* 是否可写 */
  if (!Readable(file->mode))
    return 0;
  /* 指针是否指到文件末尾 */
  if (file->buff[0].eof) {
    file->buff[0].offset = 0;
    file->buff[0].buff_size = 0;
    return 0;
  }
  /* 从文件中读取FILE_BUFF_SIZE个字节到缓存 */
  size_t size =
      fread(file->buff[0].data, sizeof(char), FILE_BUFF_SIZE, FP(file));
  if (size < FILE_BUFF_SIZE) {
    /* 文件内容小于默认的缓存大小，即问价内容已经全部读取到缓存 */
    file->buff[0].buff_size = size;
    file->buff[0].eof = 1;
  } else {
    /* 读取一部分内容到缓存 */
    file->buff[0].buff_size = FILE_BUFF_SIZE;
  }
  /* 重置缓存内指针位置 */
  file->buff[0].offset = 0;
  return size;
}

/**
 * @brief 打开文件
 *
 * @param file
 * @return void*
 */
void *plain_open(XFile *file) {
  FILE *fp;
  if ((fp = fopen(file->path, file->mode_str)) == NULL) {
    return NULL;
  }
  /* 初始化一些参数 */
  file->fp = fp;

  /* 修改文件为打开状态 */
  file->open = 1;

  /* 由于不同的模式，文件打开后句柄指针指向
   * 的位置不同，所以这里使用ftell来初始化
   * 文件指针位置
   **/
  file->offset = ftell(fp);

  /* 文件长度 */
  file->length = plain_length(file);

  /* 通过文件内容长度来判断文件是否为空 */
  file->blank = plain_length(file) == 0;

  /* 检测文件的换行符 */
  plain_breakline(file);

  /* 自动读取缓存，如果是可读的话 */
  plain_auto_read(file);
}

/**
 * @brief 关闭文件
 *
 * @param file
 * @return int
 */
int plain_close(XFile *file) {
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  /* 在关闭文件之前，先将文件的缓存释放 */
  if (file->buff[0].data) {
    free(file->buff[0].data);
    file->buff[0].offset = 0;
    file->buff[0].buff_size = 0;
    file->buff[0].data = NULL;
  }
  return fclose(fp);
}

/**
 * @brief 从文件中读取N个字节到buff中
 *
 * @param file
 * @param size
 * @param buff
 * @return size_t
 */
size_t plain_read(XFile *file, size_t size, char *buff) {
  if (!Readable(file->mode))
    return 0;
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  /* 如果当前文件的指针已经指到了文件末尾，则直接返回0 */
  if (file->offset == file->length) {
    return 0;
  }

  /**
   * 当需要读取的字节数大于文件长度时
   * size使用文件长度，否则为原来的size
   */
  size = size > file->length ? file->length : size;

  /* 在当前的文件的缓存中还有多少没有读的 */
  size_t left_data_length = file->buff[0].buff_size - file->buff[0].offset;

  /**
   * 如果需要读取的size大于文件的length，并且文件内容被全部读取到
   * 缓存中，这里直接将缓存中的内容写入目标buff中，并返回文件长度
   */
  if (size >= left_data_length && left_data_length == file->length) {
    memcpy(buff, (file->buff[0].data + file->buff[0].offset), size);
    file->offset += size;
    file->buff[0].offset += size;
    return left_data_length;
  }

  /**
   * 当所读取的文件内容小于文件缓存中的内容长度时
   * 从缓存中取size个字节写入目标buff
   */
  if (size < left_data_length) {
    memcpy(buff, (file->buff[0].data + file->buff[0].offset), size);
    file->offset += size;
    file->buff[0].offset += size;
    return size;
  }

  /**
   * 当读取的size大于缓存内容时，先将当前缓存中的内容写入目标buff
   * 随后在从文件中读取内容到文件buff，并从上次目标buff最后一个字
   * 节处继续向后写，直到写入size个字节到目标buff
   */
  size_t finish = 0;
  while (finish < size) {

    /* 从当前缓存中可以向目标buff中写入的字节数 */
    if ((size - finish) < file->buff[0].buff_size) {
      left_data_length = size - finish;
    } else {
      left_data_length = file->buff[0].buff_size;
    }

    /* 目标buff的写入位置 */
    char *buff_ptr = file->buff[0].data + file->buff[0].offset;
    memcpy(buff + finish, buff_ptr, left_data_length);

    /* 更新文件的指针信息 */
    file->offset = file->offset + left_data_length;
    file->buff[0].offset += left_data_length;
    finish += left_data_length;

    /* 及时从文件中读取内容到缓存 */
    if (file->buff[0].offset == file->buff[0].buff_size) {
      plain_auto_read(file);
    }
  }

  return size;
}

size_t plain_write(XFile *file, size_t size, char *buff) {
  if (!Writeable(file->mode))
    return 0;
  void *fp = FP(file);
  if (fp == NULL) {
    return 0;
  }
  return fwrite(buff, sizeof(char), size, fp);
}

size_t plain_write_format(XFile *file, char *format, ...) {
  if (!Writeable(file->mode))
    return 0;
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
  if (!Readable(file->mode))
    return NULL;

  /* 文件为空，返回NULL */
  if (!file->length)
    return NULL;
  void *fp = FP(file);
  if (fp == NULL) {
    return NULL;
  }

  /* 文件指针已经指到了文件末尾，并且当前缓存中也全部消耗完毕 */
  if (file->buff[0].eof && (file->buff[0].offset == file->buff[0].buff_size)) {
    return NULL;
  }

  /* 记录这一次readline消耗了多少字节 */
  size_t offset = 0;
  char c;

  /* 准备结果的空间 */
  char *s = malloc(MIN_BUFF_SIZE * sizeof(char) + 1);

  /* 记录s中字符的个数 */
  size_t count = 0;

  /* 获取当前文件的换行符 */
  char line_break = file->line_break == LINE_BREAK_CR ? CR : LF;

  /* 是否需要多读取一个 */
  int next = file->line_break == LINE_BREAK_LFCR ? 1 : 0;

  while (1) {

    /* 缓存中已经消耗完毕，并且文件还未读取完成，则更新缓存 */
    if (!file->buff[0].eof &&
        (file->buff[0].offset == file->buff[0].buff_size)) {
      plain_auto_read(file);
    }

    /* 消耗一个字符 */
    c = file->buff[0].data[file->buff[0].offset];
    file->buff[0].offset++;
    offset++;

    /* 判断是否为换行符 */
    if (c == line_break && file->offset < file->length - 1) {
      if (next) {
        file->buff[0].offset++;
        offset++;
      }
      break;
    }

    /* 空间不足时，进行扩容 */
    if (count == MIN_BUFF_SIZE) {
      s = realloc(s, sizeof((int)((float)MIN_BUFF_SIZE * 1.5)));
    }

    s[count] = c;
    count++;
  }

  /* 更新文件指针位置 */
  file->offset = file->offset + offset;

  /* 添加一个字符串终止，方便使用str* 函数 */
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

unsigned char parse_mode(char *mode) {
  int count = 0;
  unsigned char m = 0;
  while (count < strlen(mode)) {
    switch (mode[count]) {
    case 'r':
      Read(m);
      count++;
      break;
    case 'w':
      Write(m);
      count++;
      break;
    case '+':
      ReadWrite(m);
      count++;
      break;
    case 'b':
      Binary(m);
      count++;
      break;
    case 'a':
      Append(m);
      count++;
      break;
    default:
      break;
    }
  }
  return m;
}

XFile *xfile(char *path, char *mode) {
  XFile *file = malloc(sizeof(XFile));
  file->path = path;
  file->mode = parse_mode(mode);
  file->mode_str = mode;
  file->line_break = LINE_BREAK_UNKNOWN;
  file->open = 0;
  file->blank = 0;
  file->length = 0;
  file->offset = 0;
  file->fp = NULL;
  /* 初始化后，buff中没有数据，只有打开了文件才会有数据 */
  if (Readable(file->mode)) {
    file->buff[0].buff_size = 0;
    file->buff[0].data = malloc(sizeof(char) * FILE_BUFF_SIZE);
    file->buff[0].offset = 0;
    file->buff[0].eof = 0;
  }
  if (Writeable(file->mode)) {
    file->buff[1].buff_size = 0;
    file->buff[1].data = malloc(sizeof(char) * FILE_BUFF_SIZE);
    file->buff[1].offset = 0;
    file->buff[1].eof = 0;
  }

  return file;
}

void destory_xfile(XFile *file) {
  if (file) {
    free(file);
  }
}
