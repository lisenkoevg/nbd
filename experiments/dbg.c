#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "dbg.h"


#define MYLOG_ROOT "/home/evgen/work"

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
#ifndef DBG
  return;
#endif
  static pthread_mutex_t lock_stderr = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock (&lock_stderr);

  fprintf(stderr, "____%s thread:%04ld ", PROG_NAME, ((unsigned long) pthread_self()) % 10000);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fprintf(stderr, "\n");
  pthread_mutex_unlock (&lock_stderr);
}

void mylog(const char *format, ...) {
#ifndef DBG
  return;
#endif
  static FILE *f = NULL;
  if (!f) {
    f = fopen(MYLOG_ROOT "/" PROG_NAME ".log", "a");
    if (f)
      perror("fopen");
  }

  static pthread_mutex_t lock_log = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock (&lock_log);
  fprintf(f, "%s pid:%d tid:%04ld ", currTime(), getpid(), ((unsigned long) pthread_self()) % 10000);
  va_list args;
  va_start(args, format);
  vfprintf(f, format, args);
  va_end(args);

  fprintf(f, "\n");
  fflush(f);
  pthread_mutex_unlock (&lock_log);
}

void save_file(const void *data, size_t size, uint64_t offset, const char * suf) {
  const char *root = getenv("_TEST_DIR_");
  if (root == NULL) {
//     perror("save_file _TEST_DIR_ not set");
    return;
  }
  char dirname[PATH_MAX] = {0};
  char cmd[PATH_MAX + NAME_MAX] = {0};
  static int first = 1;
  char filename[PATH_MAX + NAME_MAX] = {0};
  char *ts = getenv("_TS_");
  sprintf(dirname, "%s/%s/%s", root, PROG_NAME, ts != NULL ? ts : "_TS__not_set");
  if (first) {
    sprintf(cmd, "mkdir -p %s\n", dirname);
    int ret = system(cmd);
    first = 0;
  }
  sprintf(filename, "%s/%010lu%s", dirname, offset, suf);
  int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  ssize_t r = write(fd, data, size);
  if (r != size) {
    dbg("%s write error %lu %lu", __PF__, size, r);
  }
  close(fd);
}

char * currTime() {
  static char __thread buf[NAME_MAX];
  time_t t;
  size_t s;
  struct tm *tm;
  t = time(NULL);
  tm = localtime(&t);
  if (tm == NULL) {
    fprintf(stderr, "localtime returns NULL");
    return NULL;
  }
  s = strftime(buf, NAME_MAX, "%Y%m%d_%H%M%S", tm);
  return (s == 0) ? NULL : buf;
}

#ifdef TEST_DBG
#define SIZE 20
int main() {
  char buf[SIZE] = { 1, 2, 3 };
  dump_buffer(buf, SIZE);
  char *pStr = buffer_to_str_wrap(buf, SIZE, 0);
  printf("%s\n", pStr);
  free(pStr);

  dbg("%s", "dbg message");
  printf("\n");
  save_file(buf, 3, 0, "_");
}
#endif
