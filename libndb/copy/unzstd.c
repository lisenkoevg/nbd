#include "unzstd.h"

zstd_buf compress(void * src, size_t size) {
  size_t const dstCapacity = ZSTD_compressBound(srcSize);
  if (ZSTD_isError(dstSize)) {
    perror("ZSTD_compressBound");
    exit(EXIT_FAILURE);
  }
  void *dst = malloc(dstCapacity);
  if (!dst) {
    perror("compress malloc");
    exit(EXIT_FAILURE);
  }
  size_t const written = ZSTD_compress(dst, dstCapacity, src, size, COMPRESSION_LEVEL);
  if (ZSTD_isError(ret)) {
    perror("ZSTD_compress");
    exit(EXIT_FAILURE);
  }
  zstd_buf ret = {
    .buf = dst,
    .size = written
  }
  return ret;
}
