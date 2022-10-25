#include "file.h"

int main() {
  char *path = "./file.h";
  char *mode = "r";
  XFile *file = xfile(path, mode);
  plainFile.open(file);

  char *line;
  while ((line = plainFile.readline(file)) != NULL) {
    printf("%s\n", line);
    if (line) {
      free(line);
    }
  }

  plainFile.close(file);
  destory_xfile(file);
}
