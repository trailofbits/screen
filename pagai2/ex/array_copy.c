#include "pagai_assert.h"

typedef double data;

void array_copy(int n, const data* const source, data* const dest) {
  int count;
  const data *a = source;
  data *b = dest;
  if (n < 0) n=0;
  if (n > 100000000) n=100000000;
  count = n;
  while(count > 0) {
    *(b++) = *(a++);
    count--;
  }
  assert(count == 0);
  assert(a - source == n);
  assert(b - dest == n);
}
