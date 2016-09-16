#include <assert.h>

extern int choice();
extern void print(int x);
int main() {
  int x = choice();
  assert(x>=10);
  assert(x<=30);
  x++;
  print(x);
  return 0;
}
