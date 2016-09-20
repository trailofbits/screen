#include <stdio.h>

void sentinel(int n) {
  int t[n], i;
  for(i=0; i<n; i++) t[i] = i;
  t[n-2] = -1;
  for(i=0; t[i]>=0; i++) {
  }
  printf("%d %d\n", n, i);
}

int main() {
  sentinel(10);
  return 0;
}
