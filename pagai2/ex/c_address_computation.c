#include "pagai_assert.h"

typedef unsigned long size_t;
extern void *calloc (size_t __nmemb, size_t __size);

typedef double data;

int main() {
  int x;
  data *p1 = calloc(10, sizeof(data)), *p2 = p1+10;
  x= p2-p1;
  assert(x==10);
}
