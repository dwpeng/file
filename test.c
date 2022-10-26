#include "file.h"

int main() {
  char *path = "./test.txt";
  char *mode = "a+";
  XFile *file = xfile(path, mode);
  plainFile.open(file);
  char buff[100] = {0};
  char *line;

  size_t size = plainFile.read(file, 5, buff);
  printf("%s\n", buff);

  while ((line = plainFile.readline(file)) != NULL) {
    printf("###> %s\n", line);
    if (line) {
      free(line);
    }
  }

  plainFile.close(file);
  destory_xfile(file);
}
