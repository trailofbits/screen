#include "../../pagai_assert.h"
int nondet();

int main ()
{
  int x = 0;
  int y = 0;
  int z = nondet();

  while (nondet()){
    x += 10;
    y += 1;
  }

  if (x <= z && y >= z + 1)
    goto ERROR;


    return 0;

  ERROR: assert(0);
}
