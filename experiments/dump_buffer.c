#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *dump_buffer_str(const void *buf, const size_t size) {
  char tmp_f[10];
  char *tmp = (char *)calloc(size * 4, 1);

  for (size_t i = 0; i != size; ++i) {
    if (i && i % 8 == 0) strcat(tmp, "\n");
    sprintf(tmp_f, "%02x ", *((unsigned char *)buf + i));
    strcat(tmp, tmp_f);
  }
  strcat(tmp, "\n");
  return tmp;
}

void dump_buffer(const void *buf, const size_t size) {
  char *s = dump_buffer_str(buf, size);
  fprintf(stderr, "%s", s);
  free((void *)s);
}

#ifdef TEST_DUMP_BUFFER
#define SIZE 50
int main() {
  char buf[SIZE] = { 1,2,3,4,5 };
  dump_buffer(buf, SIZE);
}
#endif
