#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void dump_buffer(const void *buf, size_t size) {
  char tmp_f[10];
  char *tmp = (char *)calloc(size * 4, 1);

  for (size_t i = 0; i != size; ++i) {
    if (i && i % 8 == 0) strcat(tmp, "\n");
    sprintf(tmp_f, "%02x ", *((unsigned char *)buf + i));
    strcat(tmp, tmp_f);
  }
  strcat(tmp, "\n");
  fprintf(stderr, "dump_buffer(size=%lu):\n%s\n", size, tmp);
  free(tmp);
}


#ifdef TEST_DUMP_BUFFER
#define SIZE 50
int main() {
  char buf[SIZE] = { 1,2,3,4,5 };
  dump_buffer(buf, SIZE);
}
#endif
