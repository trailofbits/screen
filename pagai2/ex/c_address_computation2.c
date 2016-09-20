#include "pagai_assert.h"

typedef unsigned long size_t;
extern void *calloc (size_t __nmemb, size_t __size);
extern int nondet(void);

typedef double data;

int main() {
  int x;
  data *p1 = calloc(10, sizeof(data)),
    *p2 = p1+10,
    *p3 = p2+5,
    *p4 = nondet() ? p2 : p3;
  x= p4-p1;
  assert(x>=10);
}
