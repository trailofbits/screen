#include "pagai_assert.h"

typedef unsigned long size_t;
extern void *calloc (size_t __nmemb, size_t __size);

typedef double data;

int main() {
  int x = 0;
  data *begin = calloc(10, sizeof(data)), *end = begin+10;
  for(data *p = begin; p<end; p++) x++;
  assert(x==10);
}
