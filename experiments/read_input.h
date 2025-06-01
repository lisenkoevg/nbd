#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read arbitrary lenght input from stdin.
// Returns C-style string.
// Returns null if memory allocation error happens.
// Result supposed to be freed by client.
char *read_input() {
  size_t sz = 1;
  char *ret = (char *)malloc(sz);
  if (ret == NULL)
    return ret;
  size_t i = 0;
  int ch;
  while ((ch = getchar()) != EOF) {
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
  }
  if (i + 1 < sz) {
    char *tmp = (char *) realloc(ret, i + 1);
    ret = tmp;
  }
  *(ret + i) = '\0';
  return ret;
}
