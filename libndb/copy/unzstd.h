#include <zstd.h>

#define COMPRESSION_LEVEL 3

typedef struct zstd_buf {
  void *buf;
  size_t size;
} zstd_buf;

zstd_buf compress(void *src, size_t size);

