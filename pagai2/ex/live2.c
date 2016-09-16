#include <assert.h>

extern int choice();
extern void print(int x);
int main() {
  int x = choice();
  assert(x>=10);
  assert(x<=30);

  for (int i = 0; i < 10; i++)
	x++;
  print(x);
  return 0;
}
