#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zstd.h>
#include "read_input.h"

#define SZ 1

char *read_input();

int main() {
  size_t *size;
  char *src = read_input(0, size);
//   printf("Input length: %lu bytes\n", strlen(src));
#if 1
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
