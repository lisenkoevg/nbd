#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zstd.h>

#define SZ 1

char *read_input();

int main() {
  char *src = read_input();
  printf("Input length: %lu bytes\n", strlen(src));
#if 0
  const int compressLevel = 3;
  size_t compressBound = ZSTD_compressBound(SZ);
  char *dst = malloc(compressBound);
  //   printf("compress bound for data size=%d: %lu\n", SZ, compressBound);
  size_t compressSize = ZSTD_compress(dst, compressBound, src, sizeof src, compressLevel);
  //   printf("compressed size: %lu\n", compressSize);

  //   for (size_t i = 0; i != sizeof src; ++i)
  //     printf("%c", *(src + i));

  for (size_t i = 0; i != compressSize; ++i)
    printf("%c", *(dst + i));
  //   printf("%s\n", ZSTD_versionString());
  free(dst);
#endif

  free(src);
  return EXIT_SUCCESS;
}

// Read arbitrary lenght input from stdin.
// Returns C-style string.
// Returns null if memory allocation error happens.
// Result supposed to be freed by client.
char *read_input() {
  size_t sz = 1;
  char *ret = (char *)malloc(sz);
  if (ret == NULL)
    return ret;
  size_t i = 0;
  int ch;
  while ((ch = getchar()) != EOF) {
    if (i == sz - 1) {
      sz *= 2;
      char *tmp = (char *)realloc(ret, sz);
      if (tmp == NULL) {
        free(ret);
        return NULL;
      }
      ret = tmp;
    }
    *(ret + i) = (char)ch;
    ++i;
  }
  if (i + 1 < sz) {
    char *tmp = (char *) realloc(ret, i + 1);
    ret = tmp;
  }
  *(ret + i) = '\0';
  return ret;
}
