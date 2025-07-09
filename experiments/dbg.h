#include <stdlib.h>
#include <stdint.h>

#ifndef DBG_H
#define DBG_H

#ifdef NBDKIT_COMMON_H
  #define PROG_NAME "nbdkit"
#else
#ifdef NBDCOPY_H
  #define PROG_NAME "nbdcopy"
#else
  #define PROG_NAME ""
#endif
#endif

char *buffer_to_str_wrap(const void *buf, const size_t size, int wrap);

void dump_buffer(const void *buf, const size_t size);

#define __PF__ __PRETTY_FUNCTION__
void dbg(const char *format, ...);

char * currTime();

void save_file(const void *data, size_t size, uint64_t offset, const char * suf);
void mylog(const char *format, ...);

#endif
