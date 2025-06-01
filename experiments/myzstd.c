/*
  ZSTD compress stdin to stdout.
  Compressed data differs from zstd utility:

  $ printf "\x01" | zstd | hd
  00000000  28 b5 2f fd 04 58 09 00  00 01 30 e7 21 1b        |(./..X....0.!.|
  0000000e

  $ printf "\x01" | myzstd | hd
  00000000  28 b5 2f fd 20 01 09 00  00 01                    |(./. .....|
  0000000a
*/

#include <stdio.h>
#include <zstd.h>
#include "read_input.h"

#define COMPRESSION_LEVEL 3 // default compression level according to MAN zstd(1)

int main() {
  size_t sz = 0;
  char *src = read_input(0, &sz);
#if 1
  size_t dstCapacity = ZSTD_compressBound(sz);
  char *dst = malloc(dstCapacity);
  const size_t ret = ZSTD_compress(dst, dstCapacity, src, sz, COMPRESSION_LEVEL);

  if (ZSTD_isError(ret)) {
    fprintf(stderr, "Error: %s\n", ZSTD_getErrorName(ret));
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i != ret; ++i) {
    putchar(*(dst + i));
  }
#endif
#if 0
  if (not pipe) {
    fprintf(stderr, "No write to stdout\n");
    exit(EXIT_FAILURE);
  }
#endif

}
