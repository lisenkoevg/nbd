#include <stdio.h>

int main() {
  FILE *f = fopen("file.dat", "w");
  char *s = "abc";
  fwrite(s, 2, 1, f);
  fclose(f);
  return 0;
}
