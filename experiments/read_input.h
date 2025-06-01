#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read arbitrary lenght input from stdin.
// Returns C-style string.
// Returns null if memory allocation error happens.
// Result supposed to be freed by client.
char *read_input(int add_null, size_t *size) {
  size_t sz = 1;
  char *ret = (char *)malloc(sz);
  if (ret == NULL)
    return ret;
  size_t i = 0;
  int ch = getc(stdin);
  while (!feof(stdin)) {
    if (i == sz - 1) {
      sz *= 2;
      char *tmp = (char *)realloc(ret, sz);
      if (tmp == NULL) {
        free(ret);
        return NULL;
      }
      ret = tmp;
    }
    *(ret + i) = (char)ch;
    ++i;
    ch = getc(stdin);
  }
  if (size != NULL)
    *size = i;

  if (add_null) {
    if (i + 1 < sz) {
      char *tmp = (char *) realloc(ret, i + 1);
      ret = tmp;
    }
    *(ret + i) = '\0';
  }
  return ret;
}
