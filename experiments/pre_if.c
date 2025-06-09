#include <stdio.h>

int main() {
#ifdef NBDCOPY_H
    printf("%s\n", "NBDCOPY_H");
#else
#ifdef NBDKIT_INTERNAL_H
    printf("%s\n", "NBDKIT_INTERNAL_H");
#else
   printf("%s\n", "none");
#endif
#endif
}
