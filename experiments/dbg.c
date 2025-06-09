#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

#include "dbg.h"

char *buffer_to_str_wrap(const void *buf, const size_t size, int wrap) {
  char tmp_f[10];
  char *tmp = (char *)calloc(size * 4 + 1, 1); // size * 4 + 1 - "+ 1" - if append NL at the end

  for (size_t i = 0; i != size; ++i) {
    if (wrap && i && i % wrap == 0) strcat(tmp, "\n");
    int n = sprintf(tmp_f, "%02x ", *((unsigned char *)buf + i));
    strcat(tmp, tmp_f);
  }
  return tmp;
}

void dump_buffer(const void *buf, const size_t size) {
  char *s = buffer_to_str_wrap(buf, size, 8);
  fflush(stderr);
  fprintf(stderr, "%s\n\n", s);
  fflush(stderr);
  free(s);
}

#define __PF__ __PRETTY_FUNCTION__

void dbg(const char *format, ...) {
  static pthread_mutex_t lock_stderr = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock (&lock_stderr);

#ifdef NBDKIT_COMMON_H
  fprintf(stderr, "____%s thread:%04ld ", "nbdkit", ((unsigned long) pthread_self()) % 10000);
#else
#ifdef NBDCOPY_H
  fprintf(stderr, "____%s thread:%04ld ", "nbdcopy", ((unsigned long) pthread_self()) % 10000);
#else
  fprintf(stderr, "____thread:%04ld ", ((unsigned long) pthread_self()) % 10000);
#endif
#endif

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fprintf(stderr, "\n");
  pthread_mutex_unlock (&lock_stderr);
}

#ifdef TEST_DUMP_BUFFER
#define SIZE 20
int main() {
  char buf[SIZE] = { 1, 2, 3 };
  dump_buffer(buf, SIZE);
  char *pStr = buffer_to_str_wrap(buf, SIZE, 0);
  printf("%s\n", pStr);
  free(pStr);
}
#endif
