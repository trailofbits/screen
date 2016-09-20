#include <assert.h>

extern void print(int x);

void trip_counts() {
  int a=0, b=0, c=0, d=0, e=0, i;
  for(i=0; i<100; i++) {
    assert (4*b <= a + 16);
    a++;
    if (i%4 == 0) b++;
    else c++;
    if (i%4 == 0) d++;
    if (i%2 == 0) e++;
  }
  print(i);
  print(a);
  print(b);
  print(c);
  print(d);
  print(e);
}
