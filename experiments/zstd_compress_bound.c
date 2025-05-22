#include <stdio.h>
#include <zstd.h>

int main() {
  for (size_t i = 0, sz = 1; i != 24; ++i, sz = 1 << i) {
    printf("ZSTD_compressBound(%lu) = %lu\n", sz, ZSTD_compressBound(sz));
  }
}
