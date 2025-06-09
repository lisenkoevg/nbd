#ifndef DBG_H
#define DBG_H

char *buffer_to_str_wrap(const void *buf, const size_t size, int wrap);

void dump_buffer(const void *buf, const size_t size);

#define __PF__ __PRETTY_FUNCTION__
void dbg(const char *format, ...);

#endif
